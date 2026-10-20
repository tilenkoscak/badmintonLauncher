# GPIO pin map

**v0.5 (2026-10-20).** Every signal between the ESP32-S3 and the rest of the machine.
The firmware's pin definitions, `PARTS.md` and `wiring-diagrams.html` must match this table;
change all of them together.

Header positions use the DevKitC-1 v1.1 numbering, which Tilen's board matches (checked
against a photo of its silkscreen, 2026-10-20). Hold the board with the antenna up and the
USB-C ports down: **J1 is the left row, J3 the right row, and pin 1 is at the antenna end**
(J1-1 = 3V3, J3-1 = GND). `wiring-diagrams.html` draws the board this way.

The stepper, servo, wheel-PWM and feeder-endstop pins are final. GPIO 2, 42, 41 and 40 are
reservations for inputs that do not exist yet.

## Assigned pins

| GPIO | Header | Name | Dir | Connects to | Boot-state resistor | Notes |
|---|---|---|---|---|---|---|
| 4 | J1-4 | `PAN_STEP` | out | DRV8825 #1 STEP | - | rising edge = 1 microstep |
| 5 | J1-5 | `PAN_DIR` | out | DRV8825 #1 DIR | - | |
| 6 | J1-6 | `TILT_STEP` | out | DRV8825 #2 STEP | - | |
| 7 | J1-7 | `TILT_DIR` | out | DRV8825 #2 DIR | - | |
| 15 | J1-8 | `FEED_STEP` | out | DRV8825 #3 STEP (feeder carriage) | - | |
| 16 | J1-9 | `FEED_DIR` | out | DRV8825 #3 DIR | - | |
| 17 | J1-10 | `STEP_EN` | out | ENABLE of all three DRV8825 | **10 kΩ pull-up to 3V3** | active-low. Pull-up keeps steppers de-energised until firmware drives it LOW |
| 18 | J1-11 | `STEP_FAULT` | in | FAULT of all three DRV8825, wire-OR | internal `INPUT_PULLUP` | optional. Open-drain, LOW = fault. Verify idle ≤ 3.3 V first |
| 8 | J1-12 | *(reserved)* | - | - | - | default I²C SDA, keep for a display/IMU later |
| 9 | J1-15 | *(reserved)* | - | - | - | default I²C SCL |
| 10 | J1-16 | `SERVO_ARMS` | out (PWM 50 Hz) | arms servo (geared gripper), Miuzei 15 kg | - | OPEN 145° / CLOSED 154° (defaults, adjustable in the app) |
| 11 | J1-17 | `SERVO_LIFT` | out (PWM 50 Hz) | lift servo (rack and pinion), Miuzei 15 kg; replaces the Uno-era spoon servo | - | BOTTOM 160° / TOP 0° (defaults, adjustable in the app) |
| 12 | J1-18 | `WHEEL_L_PWM` | out (PWM, `WHEEL_PWM_HZ` ≤ 20 kHz) | PWM pin of MOSFET module L | **10 kΩ pull-down to GND** | wheel off during boot. Duty capped by `WHEEL_MAX_DUTY` |
| 13 | J1-19 | `WHEEL_R_PWM` | out (PWM) | PWM pin of MOSFET module R | **10 kΩ pull-down to GND** | same duty as L, started a moment after L |
| 14 | J1-20 | *(spare)* | - | - | - | the MOSFET modules have no enable pin. Reserved for a future relay on the aim & feed rail |
| 1 | J3-4 | `FEED_HOME` | in | optical endstop SIGNAL, homes the feeder carriage | none: the board drives the line | `FEED_HOME_ACTIVE_LEVEL = LOW`. Board powered from 3V3, see `06-optical-endstop.md` |
| 2 | J3-5 | `LIMIT_PAN` | in | pan home sensor, **only if one is added later** | internal `INPUT_PULLUP` | not planned: pan/tilt START is confirmed at a frame mark after power-up (`controller/README.md`) |
| 42 | J3-6 | `LIMIT_TILT` | in | tilt home sensor, only if added later | internal `INPUT_PULLUP` | |
| 41 | J3-7 | `ESTOP_IN` | in | auxiliary contact on the wheel-rail E-STOP (optional) | internal `INPUT_PULLUP` | lets the app show "wheel power off" |
| 40 | J3-8 | `SHUTTLE_SENSE` | in | empty-tube sensor (optional, future) | internal `INPUT_PULLUP` | |
| 48 | J3-16 | `RGB_LED` | out | onboard RGB LED | - | status: booting / AP up / client connected / e-stop. GPIO 38 on some boards; smoke test 1 in `01-esp32-s3-basics.md` tells |

Spare, free for later: **14, 21, 38, 39, 47** (38 only if the LED is not on it).

## Pins that must stay unconnected

| GPIO | Reason |
|---|---|
| 35, 36, 37 | Octal PSRAM of the N16R8 module. Never connect anything |
| 19, 20 | Native USB D−/D+ |
| 43, 44 | UART0 TX/RX to the USB-serial chip (uploads, serial monitor) |
| 0, 3, 45, 46 | Strapping pins, level at reset selects boot mode |

## Power pins used

| Pin | Use |
|---|---|
| `3V3` | DRV8825 RESET + SLEEP (×3), pull-up for `STEP_EN`, M0–M2 jumpers, optical endstop VCC. Nothing that draws real current |
| `5Vin` (J1-21) | Board supply input from the MEAN WELL N7805 (24 V → 5 V) when not on USB |
| `GND` (any) | Logic ground to the drivers' logic-GND pins, the MOSFET module header GNDs, the endstop GND, and one wire to the supply's star point |

## Microstepping jumpers (not GPIOs)

The Uno code drove M0/M1 of the drivers from one GPIO. On the ESP32 they are jumpered per
driver and the resolution is a constant per axis in `config.h`. All three drivers run at
**1/8**: the gearboxes already give pan and tilt far more resolution than aiming needs
(162.5 and 320 microsteps per degree, `02-steppers-drv8825.md`), and 1/8 halves the pulse
rate compared with 1/16.

| Driver | Prototype setting | ESP32 build | M0 | M1 | M2 |
|---|---|---|---|---|---|
| Pan | M0+M1 on Uno pin 9, software ×2 | 1/8 | 3V3 | 3V3 | open |
| Tilt | M0+M1 on Uno pin 9, software ×2 | 1/8 | 3V3 | 3V3 | open |
| Feeder | 1/8 (M0+M1 HIGH) | 1/8 | 3V3 | 3V3 | open |

Taught positions are stored in microsteps, so a jumper change after teaching means
re-teaching that axis.

## Firmware constants tied to this map

| Constant | Meaning | Where it comes from |
|---|---|---|
| `WHEEL_RAIL_V` | Wheel rail voltage the firmware assumes: 24 until the 12 V buck is fitted and measured, then 12 | `04-launch-wheels-dc-motors.md` |
| `WHEEL_MAX_DUTY` | Duty cap for GPIO 12/13, derived: min(100 %, 12 V / `WHEEL_RAIL_V`) | `04-launch-wheels-dc-motors.md` |
| `WHEEL_PWM_HZ` | LEDC frequency for the MOSFET modules, ≈ 16 kHz, ≤ 20 kHz (Uno used 980 Hz) | module spec |
| `FEED_HOME_ACTIVE_LEVEL` | Level on GPIO 1 that means "carriage at home": LOW on the prototype | `06-optical-endstop.md` |
| `FEED_HOME_DIR` | Direction of the homing move: towards the wheels (the endstop sits at the front of the carriage travel) | `06-optical-endstop.md` |
| `SERVO_MIN_US`, `SERVO_MAX_US` | Pulse range for 0-180°: 544 and 2400 µs, the Uno `Servo` library's range, so Tilen's angles carry over | `03-servos.md` |
| `SERVO_MAX_DEG` | 160: every servo command is clamped to 0-160° | `03-servos.md` |
| Arms OPEN/CLOSED, lift BOTTOM/TOP, settle times | Defaults in `config.h`: 145°/154°, 160°/0°, 300 ms arms, 800 ms lift. The values in use are adjusted in the app and stored in flash | `03-servos.md` |
| `MICROSTEPS_PAN`, `_TILT`, `_FEED` | 8 each, must match the M0-M2 jumpers above | this file |
| `PAN_GEAR_RATIO`, `TILT_GEAR_RATIO` | 36.57 (approximate) and 72; only used to show degrees | `02-steppers-drv8825.md` |
| Pan/tilt START, END | Microsteps; taught in the app and stored in flash | `controller/README.md` |
| Carriage PARK, TUBE, WHEELS | Microsteps from home; taught in the app and stored in flash | `controller/README.md` |

## Why these pins

- GPIO 4–7 and 15–18 are eight consecutive header pins on J1: all stepper signals on one
  ribbon cable.
- Servo and wheel PWM on 10–13, the next block down on J1.
- Inputs on J3 (1, 2, 42, 41, 40) near the top of the right column, away from the power pins.
- Nothing on strapping pins, USB pins, UART0 pins or PSRAM pins.
- GPIO 1 and 2 are ADC1 pins, so a homing switch can later be replaced by a potentiometer
  or Hall sensor without rewiring.
