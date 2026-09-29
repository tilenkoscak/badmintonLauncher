# 03 · The two feeder servos

**Miuzei 15 kg digital servo**, metal gear, 180° (Amazon.de B0C4T73SMB). Operating voltage
**4.8–7.4 V**, pulse 0.5–2.5 ms at 50 Hz, 0.13 s per 60°, stall current not published
(assume 2–3 A each when stalled).

| Servo | Signal GPIO | Uno pin it replaces |
|---|---|---|
| Arms servo (`SERVO_ARMS`) | GPIO 10 | 10 |
| Spoon servo (`SERVO_SPOON`) | GPIO 11 | 11 |

Roles and working positions come from the Uno code (`controller/reference-uno/README.md`):

| Servo | Closed | Open | Other | Settle delays used |
|---|---|---|---|---|
| Arms | 10° | 40° | – | open 550 ms, close 300 ms |
| Spoon | 180° | 0° | guiding position 105° | open 600 ms, close 750 ms, guiding 600 ms |

These are Uno `Servo` degrees, i.e. pulses between 544 and 2400 µs. Use the same range in
ESP32Servo (`attach(pin, 544, 2400)`) or the positions shift by a few degrees.

> ❓ **OPEN:** Still needed: what the arms and the spoon physically do to a shuttle
> (`PARTS.md` #4), so the firmware can name states and detect a failed feed.

## What changes compared with the Uno

- The Uno's `Servo.h` does not exist for ESP32. Use the **ESP32Servo** library
  (Library Manager → "ESP32Servo" by Kevin Harrington). Same `attach()` / `write()` API.
- Signal is 3.3 V instead of 5 V. Digital servos of this class accept it. If a servo ignores
  the ESP32 but worked on the Uno, that is the one exception, and a single transistor or a
  74HCT125 buffer fixes it.
- Servo power must **not** come from the ESP32 board and, in this build, not from the same
  regulator as the ESP32 either.

## Power

The prototype feeds the servos from the 24 V rail through a **MEAN WELL N7805-1CW**
(5 V, **1 A**; confirmed to be the 5 V part, which is right for these servos). Two 15 kg
servos can pull several amps for a moment; when they do, the N7805's overload protection
drops the rail and the servos twitch. Recommended split, see `05-power.md`:

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
  Arms servo  │                               │  Spoon servo
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
| Orange | ESP32 GPIO 10 (arms) or GPIO 11 (spoon) |

- The 470–1000 µF capacitor across the servo rail absorbs the current spike when a servo
  starts or stalls.
- Route servo power wires away from the ESP32 board. Only the signal wires and one ground go
  to the ESP32 side.

## Smoke test: open and close both servos

Servo rail on, ESP32 on USB, common GND connected. **If the servos are mounted in the
feeder, retract the feeder first** and use only the angles below; the Uno code warns that
90° (where a servo goes after a bare `attach()`) makes the arms hit an obstacle and the spoon
block the feeder. That is why each servo is written to its closed position in the same line
it is attached.

```cpp
#include <ESP32Servo.h>

Servo arms, spoon;

void setup() {
  arms.setPeriodHertz(50);
  spoon.setPeriodHertz(50);
  arms.attach(10, 544, 2400);  arms.write(10);     // Uno Servo pulse range, closed at once
  spoon.attach(11, 544, 2400); spoon.write(180);   // closed at once
  delay(1000);
}

void loop() {
  arms.write(40);   delay(550);   // arms open
  arms.write(10);   delay(300);   // arms closed
  spoon.write(105); delay(600);   // spoon guiding position
  spoon.write(0);   delay(600);   // spoon open
  spoon.write(180); delay(750);   // spoon closed
  delay(1500);
}
```

Expected: the same open/close pattern as the prototype's reload cycle, minus the feeder
stroke. Jitter or twitching = servo rail too weak or missing capacitor. A servo that never
moves = check the signal pin number and that the servo GND is connected to the ESP32 GND.

Note for the firmware: ESP32Servo and the wheel PWM both use the LEDC hardware timers.
The ESP32-S3 has 8 channels, this project uses 4 (2 servos + 2 wheels), so there is no
conflict as long as servos are attached before other PWM is configured, or timers are
assigned explicitly.
