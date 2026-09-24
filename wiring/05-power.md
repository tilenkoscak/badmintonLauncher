# 05 · Power distribution

**Status: blocked** on the power supply specification. The structure below is what the
finished robot will need regardless of the exact supply; the numbers are estimates.

> ❓ **OPEN:** Before I can finish this page I need the supply's voltage and current rating,
> its type (bench PSU, brick, LED PSU, battery) and whether the robot must run from a battery
> on court (`PARTS.md` #7). I also need the servo model (#4) to fix the servo rail voltage and
> the confirmed 775 driver (#6) to fix the fuse ratings.

## Power tree (planned)

```
  Mains adapter / bench PSU / battery
             │  12 V
             ▼
   ┌─────────────────┐   MOTOR SWITCH / E-STOP        FUSE 15–20 A      775 driver L ──► 775 motor L
   │  12 V supply    ├──────────/ ──────┬──────────────[====]──────────►
   │  (?? A)         │                  ├──────────────[====]──────────►  775 driver R ──► 775 motor R
   └───────┬─────────┘                  │            FUSE 15–20 A
           │                            │
           │                            ├────── FUSE 5 A ───────────────►  DRV8825 VMOT ×3 (+100 µF each)
           │                            │
           │                            └────► 5 V buck ≥ 3 A ──(+1000 µF)──►  servo rail → servo A, servo B
           │
           └────────────────────────────────► 5 V buck ≥ 1 A ───────────────►  ESP32 `5V` pin
                                                                                (USB from the PC while developing)

   GROUND: one star point at the supply. Each branch (wheel L, wheel R, steppers, servo buck,
   ESP32 buck) has its own wire back to the star. The ESP32's GND pins connect only to the
   star and to the drivers' logic-GND pins.
```

Why the ESP32 sits **before** the motor switch: pressing E-STOP must kill the wheels,
steppers and servos while the ESP32 stays alive, so the phone keeps its connection and the
app can show "E-STOP pressed" instead of just going dark.

## Rails

| Rail | Voltage | Feeds | Estimated current |
|---|---|---|---|
| Motor rail | 12 V | 775 motors, DRV8825 VMOT | 2 × 12.5 A at rated load, briefly far more at spin-up; steppers up to ~1.5 A each from VMOT (actual supply draw is lower thanks to the chopper) |
| Servo rail | 5 V (6 V if the servos allow it, more torque) | 2 servos | 0.5 A idle, 2–5 A during stalls, depending on model |
| Logic rail | 5 V into the ESP32 board, 3.3 V out of its LDO | ESP32, DRV8825 logic (RESET/SLEEP, pull-ups) | ≤ 0.6 A |

Preliminary supply size: **12 V, 30 A (360 W)** mains supply, or a **3S Li-ion / LiPo pack**
(11.1–12.6 V) with a BMS and connectors rated ≥ 40 A if the robot must be portable.
Battery voltage sag directly changes wheel speed, which is one reason to add RPM feedback later.

## Rules

- **Power-up order:** ESP32 first, then the motor switch. Power-down: motor switch first.
  The pull-up on stepper ENABLE and pull-downs on wheel PWM (see `pin-map.md`) cover the
  case where this order is not respected.
- **Never route motor current through the ESP32 board** or through a breadboard. Breadboard
  rails are good for ~1 A; 775 currents melt them.
- **Capacitors:** 100 µF at every DRV8825, ≥ 1000 µF at the servo rail, whatever the 775
  driver module recommends at its input (a BTS7960 module likes 470–1000 µF nearby).
- **Fuses** on each 775 branch and on the stepper branch. Ratings to be confirmed after
  measuring real currents.
- **Wire gauge:** ≥ 1.5 mm² for the wheel branches and the main 12 V feed (2.5 mm² if the
  run is long), 0.5 mm² is enough for steppers and servos, signal wires can be thin.
- **Soft start** for the 775 motors in firmware keeps the supply from tripping its
  over-current protection at start-up.
- **Brown-out:** if the ESP32 resets when motors start, its 5 V supply is sagging. Separate
  buck, shorter/thicker ground, more capacitance.
- **Battery monitoring** (if battery): a resistor divider from the 12 V rail to an ADC1 pin
  (GPIO 1–10) lets the app show battery voltage. Not for v1.
