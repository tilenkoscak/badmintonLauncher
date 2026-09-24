# Wiring the launcher to the ESP32-S3

This folder explains how to connect every motor and driver of the prototype to the
ESP32-S3 board, and what is different from doing the same with an Arduino Uno.
Read the files in numerical order the first time. Each one ends with a small smoke test
so a subsystem is proven on the ESP32 before the next one is connected.

| File | Subsystem | State |
|---|---|---|
| `01-esp32-s3-basics.md` | The ESP32-S3 board itself: which pins may be used, power, Arduino IDE setup | Draft. Board photo pending |
| `02-steppers-drv8825.md` | Pan, tilt and feeder steppers through DRV8825 | Draft. Motor specs pending |
| `03-servos.md` | The two feeder servos | Draft. Servo model pending |
| `04-launch-wheels-dc-motors.md` | The two 775 motors and their PWM driver | **Blocked**: driver unknown |
| `05-power.md` | Power supply, rails, grounding, switching | **Blocked**: supply unknown |
| `pin-map.md` | One table with every GPIO in use | Draft v0.1 |

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
   DRV8825 (pan)      DRV8825 (tilt)     DRV8825 (feed)     Servo A, Servo B    DC driver L / R
        │                  │                  │                  │             (type: OPEN)
        ▼                  ▼                  ▼                  ▼                  ▼
   Pan stepper        Tilt stepper       Feed stepper       5–6 V servo rail    775 motor L / R
                                                                                 (12 V, up to
   ◄─────────────── VMOT from 12 V rail ───────────────►                          tens of amps)

   Power: 12 V supply ─► motor rail (775 drivers, DRV8825 VMOT)
                      ─► 5 V buck  ─► servos
                      ─► 5 V buck  ─► ESP32 (or USB while developing)
   All grounds meet at one point. Motor current never passes through the ESP32 board.
```

## Rules that apply on every page

1. **3.3 V logic.** Never connect 5 V, 12 V or a driver's VMOT to an ESP32 pin. Outputs from
   the ESP32 (STEP, DIR, PWM) at 3.3 V are fine for the DRV8825 and for servos.
2. **One common ground.** ESP32 GND, driver logic GND and motor supply GND must be connected,
   otherwise STEP/DIR/PWM have no reference and nothing works or things get damaged.
3. **Motor current never flows through the ESP32 board.** The ESP32's GND pins carry only
   signal return current. Motors get their own ground wires to the supply.
4. **Never plug or unplug a stepper while the driver has VMOT.** It kills DRV8825s instantly.
5. **Power-up order:** ESP32 first (so outputs are defined), then motor rail. Power-down in
   reverse. The pull-up on the stepper ENABLE line and pull-downs on the wheel PWM lines
   (see `pin-map.md`) keep everything off until the firmware takes control.
6. **Use GPIO numbers**, not Uno-style `D4`. On the ESP32 the number printed on the board is
   the number used in code.
7. **Keep motor wires away from signal wires** and away from the ESP32's Wi-Fi antenna.
   Twist each motor's wire pair. The 775 wires carry big, noisy currents.
8. **Wire one subsystem at a time** and run its smoke test before adding the next.
9. **Guard the launch wheels.** Never test them with the disks mounted until the driver and
   e-stop are proven with bare motor shafts.
10. **Label every connector** (pan, tilt, feed, servo A/B, wheel L/R). Photograph the result
    and add the photo to this folder.

## Recommended build order

1. ESP32 board alone: install the Arduino core, blink the RGB LED, print to serial, run the
   Wi-Fi scan example (`01-esp32-s3-basics.md`).
2. One DRV8825 + the pan stepper. Then tilt, then feeder (`02-steppers-drv8825.md`).
3. Both servos on their own 5 V rail (`03-servos.md`).
4. Launch wheels, only after the driver question is answered, motors first tested with no
   disks mounted (`04-launch-wheels-dc-motors.md`).
5. Final power distribution, fuses, e-stop (`05-power.md`).
