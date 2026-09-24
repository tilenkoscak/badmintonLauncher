# 02 · Steppers: pan, tilt and feeder through DRV8825

Three identical circuits. Only the STEP/DIR pins differ (see `pin-map.md`):

| Axis | STEP | DIR | ENABLE | FAULT (optional) |
|---|---|---|---|---|
| Pan | GPIO 4 | GPIO 5 | GPIO 17 (shared) | GPIO 18 (shared) |
| Tilt | GPIO 6 | GPIO 7 | GPIO 17 (shared) | GPIO 18 (shared) |
| Feeder | GPIO 15 | GPIO 16 | GPIO 17 (shared) | GPIO 18 (shared) |

## What changes compared with the Uno

- RESET and SLEEP were probably tied to the Uno's **5 V**. On the ESP32 tie them to **3V3**.
  The DRV8825 is happy with either, but the FAULT pin idles at whatever SLEEP is tied to,
  and 5 V on an ESP32 input is not allowed.
- STEP/DIR at 3.3 V are fine: the DRV8825 treats anything above 2.2 V as high.
- The step pulses themselves are the same idea. The ESP32 can also generate them in hardware
  (FastAccelStepper uses the RMT/MCPWM peripherals), which frees the CPU for Wi-Fi.

## Connections

```
                    DRV8825 carrier, top view, potentiometer at the top
                    ┌──────────────────────────────────┐
   ESP32 GPIO 17 ───┤ ENABLE                      VMOT ├─── +12 V motor rail ─┐
   3V3 / open   ────┤ M0                           GND ├─── motor rail GND    ├─ 100 µF ≥35 V, as close
   3V3 / open   ────┤ M1                            2B ├─── coil B            │  to the pins as possible
   3V3 / open   ────┤ M2                            2A ├─── coil B           ─┘
   3V3          ────┤ RESET                         1A ├─── coil A
   3V3          ────┤ SLEEP                         1B ├─── coil A
   ESP32 STEP   ────┤ STEP                       FAULT ├─── (optional) ESP32 GPIO 18, INPUT_PULLUP
   ESP32 DIR    ────┤ DIR                          GND ├─── ESP32 GND  (logic ground)
                    └──────────────────────────────────┘
```

| DRV8825 pin | Connect to | Notes |
|---|---|---|
| VMOT | +12 V motor rail | 8.2–45 V allowed. Put the 100 µF electrolytic directly across VMOT/GND on every driver |
| GND (beside VMOT) | Motor rail ground | Both GND pins are internally connected; this one carries the motor current |
| 2B, 2A | Stepper coil B | Swapping the two wires of **one** coil reverses direction |
| 1A, 1B | Stepper coil A | Find pairs with a multimeter: a coil measures a few ohms, wires of different coils read open |
| FAULT | ESP32 GPIO 18 (optional) | Open-drain, low on over-current / over-temperature. Measure that it idles ≤ 3.3 V before connecting. Wire-OR the three drivers to one input |
| GND (beside FAULT) | ESP32 GND | Logic ground. This is what makes ESP32 and driver share a reference |
| ENABLE | ESP32 GPIO 17, plus **10 kΩ pull-up to 3V3** | Active-low. The chip has a weak pull-down, so a floating ENABLE = motor energised. The pull-up keeps drivers off until firmware pulls the line low |
| M0, M1, M2 | 3V3 or open | Microstepping, see table. Open = low. Wire them per driver; they can differ between axes |
| RESET | 3V3 | Must be high or the driver does nothing |
| SLEEP | 3V3 | Must be high. Tying RESET and SLEEP together and to 3V3 is the standard trick |
| STEP | ESP32 GPIO 4 / 6 / 15 | One rising edge = one (micro)step. Pulse ≥ 1.9 µs high and low |
| DIR | ESP32 GPIO 5 / 7 / 16 | Change only while STEP is idle |

## Microstepping (M0, M1, M2)

| M0 | M1 | M2 | Resolution | Steps per rev (1.8° motor) |
|---|---|---|---|---|
| L | L | L | full | 200 |
| H | L | L | 1/2 | 400 |
| L | H | L | 1/4 | 800 |
| H | H | L | 1/8 | 1 600 |
| L | L | H | 1/16 | 3 200 |
| H | L | H | 1/32 | 6 400 |
| L | H | H | 1/32 | 6 400 |
| H | H | H | 1/32 | 6 400 |

Suggestion until the motors are known: **1/16 for pan and tilt** (smooth, quiet aiming),
**1/4 or 1/8 for the feeder** (speed matters more than smoothness). Higher microstepping
means more pulses per second; at 1/16 and 2 rev/s that is 6 400 pulses/s, trivial for
the ESP32.

## Setting the current limit

The DRV8825 needs VMOT present to produce its reference voltage, so:

1. Motor **disconnected** (safest) or connected and idle. Motor rail on. RESET/SLEEP high.
2. Multimeter: black probe on GND, red probe on the **metal screw of the potentiometer**
   (it is the Vref node) or on the Vref via next to it.
3. Turn the pot until Vref matches the target. With the usual 0.100 Ω sense resistors:
   `I_limit = 2 × Vref`, so `Vref = I_target / 2`.
   Example: a 1.2 A motor run at 80 % → 0.96 A → Vref ≈ 0.48 V.
4. Start around 60–70 % of the motor's rated current. Increase only if the axis skips steps.
   Above ~1 A per phase fit the heatsink, above ~1.5 A add airflow.

> ❓ **OPEN:** Before I can give concrete Vref values for each axis I need the stepper ratings
> (see `PARTS.md` #3) and the sense-resistor marking on the DRV8825 boards (`R100` vs `R200`,
> `PARTS.md` #2). If Tilen already tuned the pots on the Uno prototype, the drivers can be
> moved over as they are: the current limit does not depend on the microcontroller.

## Enable strategy

One shared ENABLE line is enough for v1:

- Firmware enables all drivers when a session starts (holding torque on pan/tilt so the aim
  does not drift while the wheels vibrate) and disables them after a few minutes idle so the
  motors and drivers stay cool.
- If the tilt axis droops under gravity when disabled, it needs to stay enabled whenever the
  machine is powered, or get a separate ENABLE pin. Decide after the first tests.

## Golden rules for the drivers

- Never connect or disconnect a motor while the motor rail is powered.
- Every driver gets its own 100 µF capacitor right at VMOT/GND. Long wires from the supply
  plus no capacitor = voltage spikes above 45 V = dead driver.
- Do not touch the pot with a metal screwdriver while powered unless you are sure it is
  isolated. A ceramic or plastic trimmer tool is safer.
- Heatsink on the chip, mounted so it does not short the pins.

## Smoke test: one stepper

Wire one driver (pan) exactly as above, motor rail on, ESP32 on USB, common GND connected.

```cpp
// Spins the pan stepper at 1000 steps/s, reversing every 2 s. Pins from pin-map.md.
const int STEP_PIN = 4, DIR_PIN = 5, EN_PIN = 17;

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);      // active-low: LOW = driver enabled
}

void loop() {
  static bool dir = false;
  digitalWrite(DIR_PIN, dir);
  for (int i = 0; i < 2000; i++) {
    digitalWrite(STEP_PIN, HIGH); delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);  delayMicroseconds(500);
  }
  dir = !dir;
}
```

Expected: the motor turns smoothly one way for 2 s, then the other way. Buzzing without
motion = a coil wire is on the wrong pair or the current limit is far too low. Nothing at
all = RESET/SLEEP not high, or ENABLE stuck high, or no common ground. Hot driver within a
minute = current limit too high.

For the real firmware the step generation moves to a library
(`FastAccelStepper` or `AccelStepper`), which adds acceleration ramps and position tracking.
