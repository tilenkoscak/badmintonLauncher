# 03 · The two feeder servos

Standard hobby servos: three wires, 50 Hz pulse control, 4.8–6 V supply.

| Servo | Signal GPIO |
|---|---|
| Servo A | GPIO 10 |
| Servo B | GPIO 11 |

> ❓ **OPEN:** Before this page can be finalized I need the servo model and what each servo
> does in the feed cycle (see `PARTS.md` #4). Model decides the supply voltage (5 V vs 6 V vs
> 7.4 V for high-voltage servos) and the size of the buck converter.

## What changes compared with the Uno

- The Uno's `Servo.h` does not exist for ESP32. Use the **ESP32Servo** library
  (Library Manager → "ESP32Servo" by Kevin Harrington). Same `attach()` / `write()` API.
- Signal is 3.3 V instead of 5 V. Practically every hobby servo accepts this. If a servo
  ignores the ESP32 but worked on the Uno, that is the one exception, and a single
  transistor or a 74HCT125 buffer fixes it.
- Servo power must **not** come from the ESP32 board. The Uno's 5 V pin could just about feed
  a small SG90; the ESP32 board's regulator cannot, and stall currents of metal-gear servos
  (2–3 A) would reset it anyway.

## Connections

```
                             +5 V (or +6 V) servo rail from the buck converter
                              │
              ┌───────────────┴───────────────┐
              │  470–1000 µF                  │
              │  across the rail              │
  Servo A     │                               │     Servo B
  ┌────────┐  │                               │  ┌────────┐
  │ brown  ├──┼── GND ──── common GND ─── GND ─┼──┤ brown  │
  │ red    ├──┘                               └──┤ red    │
  │ orange ├──── ESP32 GPIO 10        GPIO 11 ───┤ orange │
  └────────┘                                     └────────┘

  Wire colours: brown/black = GND, red = V+, orange/yellow/white = signal.
```

| Servo wire | Connect to |
|---|---|
| Brown / black | Servo rail GND, which is the common ground shared with the ESP32 |
| Red | Servo rail V+ from the buck converter (5 V for standard servos) |
| Orange / yellow / white | ESP32 GPIO 10 (A) or GPIO 11 (B) |

- The 470–1000 µF capacitor across the servo rail absorbs the current spike when a servo
  starts or stalls, so the rail does not dip.
- Route servo power wires away from the ESP32 board. Only the signal wires and one ground go
  to the ESP32 side.

## Smoke test: sweep both servos

Servo rail on, ESP32 on USB, common GND connected. Servos not yet attached to the feeder
mechanism, or mechanism free to move.

```cpp
#include <ESP32Servo.h>

Servo servoA, servoB;

void setup() {
  servoA.setPeriodHertz(50);
  servoB.setPeriodHertz(50);
  servoA.attach(10, 500, 2500);   // GPIO, min and max pulse in µs
  servoB.attach(11, 500, 2500);
}

void loop() {
  servoA.write(20);  servoB.write(160); delay(800);
  servoA.write(160); servoB.write(20);  delay(800);
}
```

Expected: both servos swing back and forth every 0.8 s, mirrored. Jitter or twitching =
servo rail too weak or missing capacitor. A servo that never moves = check the signal pin
number and that the servo GND is connected to the ESP32 GND.

Note for the firmware: ESP32Servo and the wheel PWM both use the LEDC hardware timers.
The ESP32-S3 has 8 channels, this project uses 4 (2 servos + 2 wheels), so there is no
conflict as long as servos are attached before other PWM is configured, or timers are
assigned explicitly.
