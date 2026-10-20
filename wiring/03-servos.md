# 03 · The two feeder servos: arms and lift

**Miuzei 15 kg digital servo**, metal gear, 180° (Amazon.de B0C4T73SMB). Operating voltage
**4.8–7.4 V**, pulse 0.5–2.5 ms at 50 Hz, 0.13 s per 60°, stall current not published
(assume 2–3 A each when stalled).

Both servos ride on the feeder carriage of the redesigned feeder (2026-10; cycle and park
routine in `controller/README.md`):

| Servo | Signal GPIO | Mechanism | Positions (Tilen, 2026-10-20) | Settle time (default) |
|---|---|---|---|---|
| Arms servo (`SERVO_ARMS`) | GPIO 10 | Two arms coupled by gears; closing them grabs the shuttle at the cork, at the bottom of the shuttle tube | OPEN **145°**, CLOSED **154°** | 300 ms |
| Lift servo (`SERVO_LIFT`) | GPIO 11 | A pinion on the servo drives a vertical gear rack; the rack carries the arms servo | BOTTOM **160°**, TOP **0°** | 800 ms |

The lift servo replaces the Uno-era "spoon" servo on the same pin. The Uno angles (arms
10°/40°, spoon 180°/105°/0°) belong to the old mechanism and are not reused.

The angles go into `config.h` as defaults; the app can adjust them and stores the values in
flash. The settle times were not measured. 300 ms for the 9° arms move is what the old arms
needed to close; 800 ms for the lift is the servo's 0.13 s/60° over 160°, doubled for load,
plus margin. Both are adjustable in the app. Tilen's understanding is that these servos use
**0-160°**, so the firmware clamps every command to that range (`SERVO_MAX_DEG = 160`).
Tilen found the four angles on the Uno with its `Servo` library (confirmed 2026-10-20), so
they are in the 544-2400 µs scale below.

## What changes compared with the Uno

- The Uno's `Servo.h` does not exist for ESP32. Use the **ESP32Servo** library
  (`lib_deps = madhephaestus/ESP32Servo` in `platformio.ini`). Same `attach()` / `write()` API.
- Signal is 3.3 V instead of 5 V. Digital servos of this class accept it. If a servo ignores
  the ESP32 but worked on the Uno, that is the one exception, and a single transistor or a
  74HCT125 buffer fixes it.
- Servo power must **not** come from the ESP32 board and, in this build, not from the same
  regulator as the ESP32 either.

## Pulse range

The firmware uses **544-2400 µs for 0-180°**, the range of the Uno `Servo` library (and
ESP32Servo's default). Tilen's angles were found in that scale, so they carry over unchanged.
The Miuzei accepts 500-2500 µs; the narrower range only gives up a few degrees at each end,
which the mechanism does not use.

Why it matters: arms OPEN and CLOSED are only 9° apart. Written with a 500-2500 µs range,
"154°" would land about 7° further than on the Uno and press the arms hard into the cork. The
rule: angles and pulse range always travel together.

## Power

The prototype feeds the servos from the 24 V rail through a **MEAN WELL N7805-1CW**
(5 V, **1 A**; confirmed to be the 5 V part, which is right for these servos). Two 15 kg
servos can pull several amps for a moment; when they do, the N7805's overload protection
drops the rail and the servos twitch. The redesign adds holding load: the lift servo holds
the arms servo against gravity, and the arms servo holds its grip on a cork. Recommended
split, see `05-power.md`:

| Consumer | Supply |
|---|---|
| ESP32 | the existing N7805 (needs ≤ 0.6 A) |
| Servos | a **5–6 V buck converter, ≥ 3 A**, from the 24 V rail. 6 V gives more torque and speed and is inside the servo's 4.8–7.4 V range |

If the N7805 stays on servo duty for now (it worked on the prototype), put 1000 µF on its
output and keep the ESP32 off that rail.

## Connections

```
                             +5 V (or +6 V) servo rail from the servo buck
                              │
              ┌───────────────┴───────────────┐
              │  470–1000 µF                  │
              │  across the rail              │
  Arms servo  │                               │  Lift servo
  ┌────────┐  │                               │  ┌────────┐
  │ brown  ├──┼── GND ──── common GND ─── GND ─┼──┤ brown  │
  │ red    ├──┘                               └──┤ red    │
  │ orange ├──── ESP32 GPIO 10        GPIO 11 ───┤ orange │
  └────────┘                                     └────────┘

  Wire colours: brown = GND, red = V+, orange = signal.
```

| Servo wire | Connect to |
|---|---|
| Brown | Servo rail GND, which is the common ground shared with the ESP32 |
| Red | Servo rail V+ (5–6 V) |
| Orange | ESP32 GPIO 10 (arms) or GPIO 11 (lift) |

- The 470–1000 µF capacitor across the servo rail absorbs the current spike when a servo
  starts or stalls.
- Route servo power wires away from the ESP32 board. Only the signal wires and one ground go
  to the ESP32 side.
- **Both servos move with the carriage, and the arms servo also moves up and down.** Use
  flexible extension leads with a slack loop (or a small drag chain) and strain relief at both
  ends, so the leads never pull on the servo connectors or snag on the tube.

## Smoke test: check the four angles on the ESP32

Servo rail on, ESP32 on USB, common GND, wheels off, carriage away from the tube. The sketch
sends no pulse to a servo until its first command, so nothing moves at power-up. In the
serial monitor, type a command and press Enter.

```cpp
#include <Arduino.h>
#include <ESP32Servo.h>

// Feeder servo angle check. Serial monitor at 115200 baud:
//   a 145  -> arms servo to 145° (OPEN)
//   l 160  -> lift servo to 160° (BOTTOM)
// A servo gets no pulses, and so does not move, until its first command.

const int ARMS_PIN = 10, LIFT_PIN = 11;
const int PULSE_MIN_US = 544, PULSE_MAX_US = 2400;   // Uno Servo library range; the firmware uses the same
const int MAX_DEG = 160;                             // SERVO_MAX_DEG in the firmware

Servo arms, lift;

void moveServo(Servo &s, int pin, const char *name, int deg) {
  deg = constrain(deg, 0, MAX_DEG);
  if (!s.attached()) {
    s.setPeriodHertz(50);
    s.attach(pin, PULSE_MIN_US, PULSE_MAX_US);
  }
  s.write(deg);
  Serial.printf("%s -> %d deg\n", name, deg);
}

void setup() {
  Serial.begin(115200);
  Serial.println("a <deg> = arms servo, l <deg> = lift servo");
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() < 3) return;
  int deg = cmd.substring(2).toInt();
  if (cmd[0] == 'a') moveServo(arms, ARMS_PIN, "arms", deg);
  if (cmd[0] == 'l') moveServo(lift, LIFT_PIN, "lift", deg);
}
```

Procedure:

1. On its first command a servo jumps to the commanded angle at full speed and full torque.
   Send the known positions first: `a 145` (arms OPEN), then `l 160` (lift BOTTOM). The
   firmware's start-up does the same, in the same order.
2. `l 0` (TOP) and back to `l 160`. At both ends the servo must go quiet at rest. If it hums,
   it is pushing against the mechanism: move 2-3° back from the end (`l 2`, `l 158`). A 15 kg
   servo held in stall overheats and strips the rack.
3. With a shuttle in the arms: `a 154` (CLOSED) must hold the cork firmly without humming,
   `a 145` must let it go.
4. Note roughly how long each move takes, to check the 300 ms and 800 ms defaults above.
5. If an angle had to change, write the new value into the table above.

Jitter or twitching = servo rail too weak or missing capacitor. A servo that never moves =
check the signal pin number and that the servo GND is connected to the ESP32 GND.

Later the web app gets a feeder setup screen that does the same and stores the angles in
flash, so a re-adjustment does not need re-flashing.

Note for the firmware: ESP32Servo and the wheel PWM both use the LEDC hardware timers.
The ESP32-S3 has 8 channels, this project uses 4 (2 servos + 2 wheels), so there is no
conflict as long as servos are attached before other PWM is configured, or timers are
assigned explicitly.
