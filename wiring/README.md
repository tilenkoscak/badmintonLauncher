# Wiring the launcher to the ESP32-S3

This folder explains how to connect every motor and driver of the prototype to the
ESP32-S3 board, and what is different from doing the same with an Arduino Uno.
Read the files in numerical order the first time. Each one ends with a small smoke test
so a subsystem is proven on the ESP32 before the next one is connected.

| File | Subsystem | State |
|---|---|---|
| `01-esp32-s3-basics.md` | The ESP32-S3 board itself: which pins may be used, power, Arduino IDE setup | Draft. Board photo pending |
| `02-steppers-drv8825.md` | Pan, tilt and feeder steppers (Sidewinder X3 Plus motors) through DRV8825 on 24 V | Draft. Motor labels pending |
| `03-servos.md` | The two Miuzei 15 kg feeder servos | Draft. Feed roles pending |
| `04-launch-wheels-dc-motors.md` | The two 775 motors: buck, MOSFET module with buffer and TVS, flyback diode | Draft. Rail voltage decision pending (24 V today, 12 V recommended) |
| `05-power.md` | The Artillery dual-output PSU, rails, grounding, switching | Draft. PSU label and buck decision pending |
| `06-optical-endstop.md` | The salvaged optical endstop: logic, 3.3 V, homing | Draft. Role pending |
| `pin-map.md` | One table with every GPIO in use | Draft v0.2 |

Open questions are marked `❓ **OPEN:**` inside each file (convention in `CLAUDE.md`) and
collected in the checklist at the end of `PARTS.md`.

## System overview

```
                        Wi-Fi (ESP32 is the access point)
   Phone browser  ◄──────────────────────────────────────►  ESP32-S3 N16R8
                                                             │  3.3 V logic
        ┌──────────────────┬──────────────────┬──────────────┴───┬──────────────────┐
        │ STEP/DIR/EN      │ STEP/DIR         │ STEP/DIR         │ PWM ×2           │ PWM ×2
        ▼                  ▼                  ▼                  ▼                  ▼
   DRV8825 (pan)      DRV8825 (tilt)     DRV8825 (feed)     Servo A, Servo B    MOSFET module L / R
        │                  │                  │                  │             (15 A, low-side)
        ▼                  ▼                  ▼                  ▼                  ▼
   Pan stepper        Tilt stepper       Feed stepper       5–6 V servo rail    775 motor L / R
   (NEMA 17 42-35)    (NEMA 17 42-35)    (NEMA 17)          (Miuzei 15 kg)      (12 V motors, duty-capped)

   Power: Artillery X3 Plus PSU
     output 1  24 V / 4.2 A ─► DRV8825 VMOT ×3, N7805 ─► 5 V ESP32, servo buck ─► 5–6 V servos
     output 2  36 V / 9.7 A ─► buck (24 V today, 12 V recommended) ─► [E-STOP] ─► fuse ─► MOSFET module ─► 775 (+E83-004 flyback)
   All grounds meet at one star point. Motor current never passes through the ESP32 board.
```

## Rules that apply on every page

1. **3.3 V logic.** Never connect 5 V, 24 V or a driver's VMOT to an ESP32 pin. Outputs from
   the ESP32 (STEP, DIR, PWM) at 3.3 V are fine for the DRV8825, the servos and the MOSFET
   modules (trigger 3.3–20 V). The optical endstop is the one input that needs care.
2. **One common ground.** ESP32 GND, driver logic GND, MOSFET module GND and both PSU outputs
   must be connected at the star point, otherwise STEP/DIR/PWM have no reference.
3. **Motor current never flows through the ESP32 board.** The ESP32's GND pins carry only
   signal return current. Motors get their own ground wires to the star point.
4. **Never plug or unplug a stepper while the driver has VMOT.** It kills DRV8825s instantly.
5. **Power-up order:** ESP32 first, then the wheel switch. Power-down in reverse. The pull-up
   on the stepper ENABLE line and the pull-downs on the wheel PWM lines (`pin-map.md`) keep
   everything off until the firmware takes control.
6. **Use GPIO numbers**, not Uno-style `D4`. On the ESP32 the number printed on the board is
   the number used in code.
7. **Keep motor wires away from signal wires** and away from the ESP32's Wi-Fi antenna.
   Twist each motor's wire pair. The 775 wires carry big, noisy currents.
8. **The wheel rail is 24 V for 12 V motors until the buck is swapped for a 12 V one.** While
   it is, the firmware duty cap is a safety feature, not a tuning parameter. Never test the
   wheels with a sketch that lacks it.
9. **Wire one subsystem at a time** and run its smoke test before adding the next.
10. **Guard the launch wheels.** Never test them with the disks mounted until the driver and
    e-stop are proven with bare motor shafts.
11. **Label every connector** (pan, tilt, feed, servo A/B, wheel L/R, motor + / −).
    Photograph the result and add the photo to this folder.

## Recommended build order

1. ESP32 board alone: install the Arduino core, blink the RGB LED, print to serial, run the
   Wi-Fi scan example (`01-esp32-s3-basics.md`).
2. Optical endstop on 3V3: find its logic (`06-optical-endstop.md`). Five minutes, and it
   settles the level-shifting question early.
3. One DRV8825 + the pan stepper on the 24 V rail. Then tilt, then feeder
   (`02-steppers-drv8825.md`).
4. Both servos on their own 5–6 V rail (`03-servos.md`).
5. Launch wheels with bare shafts, after measuring the buck output voltage with a meter
   (`04-launch-wheels-dc-motors.md`).
6. Final power distribution, fuses, e-stop (`05-power.md`).
