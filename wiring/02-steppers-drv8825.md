# 02 · Steppers: pan, tilt and feeder through DRV8825

Three identical circuits on the **24 V** rail (PSU output 1). The motors are NEMA 17
steppers salvaged from an Artillery Sidewinder X3 Plus, label **42SHDC1067Z-15WD** (rated
current not published). Pan drives its axis through a ≈ 1:36.57 planetary gearbox, tilt
through 1:72. Each DRV8825 already carries its 100 µF / 50 V capacitor and is set to
Vref 0.6 V. Only the STEP/DIR pins differ (see `pin-map.md`):

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
- VMOT and the current limits (Vref 0.6 V) stay as on the Uno prototype. The microstep pins,
  which the Uno drove from its pin 9, get fixed jumpers instead (1/8, see below). Otherwise
  the drivers move over as they are; only the logic-side wires change.

## Connections

```
                    DRV8825 carrier, top view, potentiometer at the top
                    ┌──────────────────────────────────┐
   ESP32 GPIO 17 ───┤ ENABLE                      VMOT ├─── +24 V rail (PSU output 1) ─┐
   3V3          ────┤ M0                           GND ├─── 24 V rail GND (star point)  ├─ 100 µF / 50 V,
   3V3          ────┤ M1                            2B ├─── coil B                      │  already fitted
   open         ────┤ M2                            2A ├─── coil B                     ─┘
   3V3          ────┤ RESET                         1A ├─── coil A
   3V3          ────┤ SLEEP                         1B ├─── coil A
   ESP32 STEP   ────┤ STEP                       FAULT ├─── (optional) ESP32 GPIO 18, INPUT_PULLUP
   ESP32 DIR    ────┤ DIR                          GND ├─── ESP32 GND  (logic ground)
                    └──────────────────────────────────┘
```

| DRV8825 pin | Connect to | Notes |
|---|---|---|
| VMOT | +24 V rail (PSU output 1) | 8.2–45 V allowed; 24 V gives better speed and torque than 12 V. The 100 µF / 50 V electrolytic across VMOT/GND is already on each driver |
| GND (beside VMOT) | 24 V rail ground, at the star point | Both GND pins are internally connected; this one carries the motor current |
| 2B, 2A | Stepper coil B | Swapping the two wires of **one** coil reverses direction |
| 1A, 1B | Stepper coil A | Find pairs with a multimeter: a coil measures a few ohms, wires of different coils read open |
| FAULT | ESP32 GPIO 18 (optional) | Open-drain, low on over-current / over-temperature. Measure that it idles ≤ 3.3 V before connecting. Wire-OR the three drivers to one input |
| GND (beside FAULT) | ESP32 GND | Logic ground. This is what makes ESP32 and driver share a reference |
| ENABLE | ESP32 GPIO 17, plus **10 kΩ pull-up to 3V3** | Active-low. The chip has a weak pull-down, so a floating ENABLE = motor energised. The pull-up keeps drivers off until firmware pulls the line low |
| M0, M1, M2 | M0 and M1 to 3V3, M2 open | 1/8 microstepping on all three drivers, see table. Open = low |
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

What the Uno prototype used (`controller/reference-uno/README.md`): the feeder driver at
**1/8** (M0 and M1 tied together and driven HIGH by one GPIO), pan and tilt with a software
multiplier of 2 while sharing that same microstep pin, so their real jumpering is uncertain.
Speeds: feeder commanded 500 RPM (the Uno actually managed about 150), pan and tilt 50 RPM
with 50 RPM/s acceleration.

ESP32 build: **1/8 on all three drivers**, M0 and M1 jumpered to 3V3, M2 open, instead of
driving them from a GPIO (`pin-map.md`). That is what the Uno's pin 9 set where it was
wired. Positions are taught in microsteps, so the setting must not change after teaching.

## Gearing and steps per degree

| Axis | Gearing (Tilen) | Microsteps per output degree at 1/8 | Example: 30°/s at the axis |
|---|---|---|---|
| Pan | 2-stage planetary, ≈ 1:36.57 | 1600 × 36.57 / 360 ≈ **162.5** | motor 183 RPM, 4 900 pulses/s |
| Tilt | 1:72 | 1600 × 72 / 360 = **320** | motor 360 RPM, 9 600 pulses/s |
| Feeder carriage | positions taught in steps | 1600 per motor revolution | 150 RPM = 4 000 pulses/s |

Why 1/8 and not 1/16: the gearboxes already give a resolution far finer than aiming needs
(0.006° per microstep on pan), and 1/16 would double the pulse rates above for no visible
gain. Even all three axes at once stay below 20 000 pulses/s, comfortable for the ESP32.

The prototype's 50 RPM is only about 8°/s of pan and 4°/s of tilt through these gearboxes.
The ESP32 can go faster; the speeds get tuned on the machine. The pan ratio is approximate,
so the app's pan degrees are approximate as well; positions and limits are stored in
microsteps and are exact.

## Setting the current limit

The DRV8825 needs VMOT present to produce its reference voltage, so:

1. Motor **disconnected** (safest) or connected and idle. Motor rail on. RESET/SLEEP high.
2. Multimeter: black probe on GND, red probe on the **metal screw of the potentiometer**
   (it is the Vref node) or on the Vref via next to it.
3. Turn the pot until Vref matches the target. With the usual 0.100 Ω sense resistors:
   `I_limit = 2 × Vref`, so `Vref = I_target / 2`.

   | Motor label says | 70 % (cool, quiet) | 80 % | 100 % (heatsink + airflow) |
   |---|---|---|---|
   | 1.2 A | Vref 0.42 V | 0.48 V | 0.60 V |
   | 1.5 A | 0.53 V | 0.60 V | 0.75 V |
   | 1.7 A | 0.60 V | 0.68 V | 0.85 V |

4. Start around 70 % of the motor's rated current. Increase only if the axis skips steps.
   Above ~1 A per phase fit the heatsink, above ~1.5 A add airflow. The feeder can usually
   run lower than pan and tilt, which helps the 4.2 A budget of the 24 V rail.

**Measured on the prototype (Tilen, 2026-10-20): Vref = 0.6 V on all three drivers.** The
motor's rated current is not published (label 42SHDC1067Z-15WD) and the sense-resistor
marking was not read; with the usual R100 that is 1.2 A per phase. These settings run the
prototype, so they stay. Check by touch after the first long session: warm is fine, too hot
to hold for a few seconds means turn Vref down by 0.05 V. At 1.2 A the drivers want their
heatsinks.

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

Wire one driver (pan) exactly as above, 24 V rail on, ESP32 on USB, common GND connected.
Artillery motors come with a JST-XH plug; the two coil pairs are found with a multimeter
(a coil reads a few ohms, wires of different coils read open).

```cpp
#include <Arduino.h>

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
