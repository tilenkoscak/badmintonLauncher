# 04 · Launch wheels: two 775 DC motors (12 V, 150 W)

**Status: blocked.** The power stage that sits between the controller and the motors is not
identified yet. Everything below that does not depend on it is already written; the actual
driver wiring diagram will be added when the driver is known.

> ❓ **OPEN:** Before I can draw the wiring for this subsystem I need the exact name (or a
> photo) of the module(s) that drove the 775 motors on the Uno prototype, how many there are,
> which PWM frequency the Uno used, and how the two motors were made to counter-rotate
> (`PARTS.md` #5 and #6). Also whether the two wheels must be independently adjustable.

| Signal | GPIO | Notes |
|---|---|---|
| Wheel L PWM | GPIO 12 | 20 kHz, 10-bit. **10 kΩ pull-down to GND** so the wheel is off while the ESP32 boots |
| Wheel R PWM | GPIO 13 | same |
| Wheel driver enable / direction | GPIO 14 | Only if the driver has such a pin (BTS7960 has R_EN/L_EN). Tentative |

## The motors

| Property | Value |
|---|---|
| Nominal voltage | 12 V |
| No-load speed | 10 000 rpm |
| Rated power | 150 W ⇒ about 12.5 A continuous at 12 V |
| No-load current | unknown, expect 1–2 A |
| Stall current | unknown, expect **30 A or more** per motor |
| Start-up | Spinning up a heavy disk from standstill draws close to stall current for a moment |

The consequences: wiring of at least 1.5 mm² (16 AWG) per motor, a fuse per motor, a driver
rated well above 12.5 A continuous, and a **soft start in firmware** so the two motors do
not both pull stall current at the same instant.

## Driver requirements (whatever the module turns out to be)

1. Accepts a **3.3 V PWM input** and switches fully on. Modules built around IRF520 or
   IRF540 ("Arduino MOSFET module") do **not**: those gates need ~10 V and stay half-on at
   3.3 V, so the MOSFET heats up and the motor is weak. If the prototype used one of these
   it must be replaced.
2. Continuous current ≥ 15–20 A per motor with a heatsink, peak ≥ 40 A.
3. Flyback path for the motor's inductive current: any H-bridge has it built in; a single
   MOSFET module needs a diode (or already has one) across the motor.
4. Direction: the disks counter-rotate, but each motor only ever runs one way. A single-
   direction driver is enough if the motor wires are simply swapped on one side.

Options if the existing driver fails requirement 1 or 2:

| Option | Current | 3.3 V logic | Reversible | Comment |
|---|---|---|---|---|
| **BTS7960 / "IBT-2" H-bridge module** | 43 A peak, ~20 A continuous with heatsink | yes | yes | Most common choice for 775 motors. Has enable pins (fail-safe) and a current-sense output |
| Logic-level MOSFET module (e.g. D4184 / AOD4184 based) | ~15 A with heatsink | yes | no | Cheap, marginal for 150 W. Needs flyback diode |
| Brushed ESC (RC car type, 60 A+) | plenty | yes (servo-style signal) | yes | Driven like a servo, has its own soft start; less direct control of duty cycle |

## ESP32 side

- PWM generated with the LEDC peripheral at **20 kHz** (above hearing, low ripple) with
  10-bit resolution (0–1023). The Uno's `analogWrite` ran at 490/980 Hz and whines; the
  ESP32 does not have to.
- One PWM line per wheel so left and right can differ (spin shots later). If wheels must
  always match, firmware just writes the same duty to both.
- Pull-down resistors on the PWM lines. During reset the ESP32 pins float, and a floating
  input on some driver modules means "full speed".
- If the driver has enable pins, they go to GPIO 14 with a pull-down too, and firmware only
  raises them after the heartbeat from the phone is alive.

## Wiring rules for the power side

- Motor wires: ≥ 1.5 mm², twisted pair per motor, as short as practical, away from the ESP32
  and its antenna and away from the servo/stepper signal wires.
- A 15–20 A automotive blade fuse per motor as a starting point. Confirm after measuring the
  real current during a launch.
- A 0.1 µF ceramic capacitor directly across each motor's terminals reduces brush noise that
  otherwise disturbs Wi-Fi and the stepper signals.
- The driver's power ground goes straight to the supply's star point, never via the ESP32.
- Mount drivers with heatsinks; the BTS7960 module gets hot at 12 A.

## Safety, non-negotiable

- **No disks mounted** for the first power tests. Bare shafts only.
- The web app's E-STOP and the connection watchdog (see `controller/README.md`) must be
  working before the disks go back on.
- Firmware ramps duty up over ~1–2 s and never jumps from 0 to a high duty.
- A physical switch or e-stop on the 12 V motor rail (`05-power.md`), reachable without
  putting a hand near the wheels.
- Disks fully guarded so a shuttle or a finger cannot enter from the side.

## Smoke test (to run once the driver is confirmed)

```cpp
// Soft-starts wheel L to ~30 % duty, holds 2 s, stops 3 s. Core 3.x LEDC API.
// Core 2.x uses ledcSetup(ch, freq, bits) + ledcAttachPin(pin, ch) + ledcWrite(ch, duty).
const int WHEEL_L_PWM = 12;

void setup() {
  ledcAttach(WHEEL_L_PWM, 20000, 10);   // pin, 20 kHz, 10-bit (0..1023)
  ledcWrite(WHEEL_L_PWM, 0);
}

void loop() {
  for (int d = 0; d <= 300; d += 5) { ledcWrite(WHEEL_L_PWM, d); delay(20); }   // ramp ~1.2 s
  delay(2000);
  ledcWrite(WHEEL_L_PWM, 0);
  delay(3000);
}
```

Expected: motor ramps up smoothly, no audible PWM whine, driver stays cool at this duty.
Motor jerks to full speed = wrong pin or driver input logic inverted. Nothing = driver not
enabled or missing common ground.

## Later improvements (not for v1)

- RPM sensor per wheel (hall sensor + magnet, or optical) and a PI loop in firmware, so a
  given "speed 60 %" always gives the same launch distance regardless of battery voltage.
- Current sense from a BTS7960 to detect a jammed shuttle.
