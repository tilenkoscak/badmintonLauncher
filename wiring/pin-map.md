# GPIO pin map

**Draft v0.3 (2026-09-28).** Every signal between the ESP32-S3 and the rest of the machine.
The firmware's pin definitions and `PARTS.md` must match this table; change all three together.

Header positions refer to the DevKitC-1 v1.1 layout drawn in `01-esp32-s3-basics.md`
(J1 = left column, J3 = right column, counted from the USB end). Confirm against the
silkscreen before soldering.

> ❓ **OPEN:** The stepper, servo, wheel-PWM and feeder-endstop pins can be considered final
> once Tilen confirms the board's header layout. GPIO 2, 42, 41 and 40 are reservations for
> inputs that do not exist yet.

## Assigned pins

| GPIO | Header | Name | Dir | Connects to | Boot-state resistor | Notes |
|---|---|---|---|---|---|---|
| 4 | J1-4 | `PAN_STEP` | out | DRV8825 #1 STEP | – | rising edge = 1 microstep |
| 5 | J1-5 | `PAN_DIR` | out | DRV8825 #1 DIR | – | |
| 6 | J1-6 | `TILT_STEP` | out | DRV8825 #2 STEP | – | |
| 7 | J1-7 | `TILT_DIR` | out | DRV8825 #2 DIR | – | |
| 15 | J1-8 | `FEED_STEP` | out | DRV8825 #3 STEP | – | |
| 16 | J1-9 | `FEED_DIR` | out | DRV8825 #3 DIR | – | |
| 17 | J1-10 | `STEP_EN` | out | ENABLE of all three DRV8825 | **10 kΩ pull-up to 3V3** | active-low. Pull-up keeps steppers de-energised until firmware drives it LOW |
| 18 | J1-11 | `STEP_FAULT` | in | FAULT of all three DRV8825, wire-OR | internal `INPUT_PULLUP` | optional. Open-drain, LOW = fault. Verify idle ≤ 3.3 V first |
| 8 | J1-12 | *(reserved)* | – | – | – | default I²C SDA, keep for a display/IMU later |
| 9 | J1-15 | *(reserved)* | – | – | – | default I²C SCL |
| 10 | J1-16 | `SERVO_ARMS` | out (PWM 50 Hz) | arms servo signal (Miuzei 15 kg; Uno pin 10) | – | closed 10°, open 40° |
| 11 | J1-17 | `SERVO_SPOON` | out (PWM 50 Hz) | spoon servo signal (Miuzei 15 kg; Uno pin 11) | – | closed 180°, guiding 105°, open 0° |
| 12 | J1-18 | `WHEEL_L_PWM` | out (PWM, `WHEEL_PWM_HZ` ≤ 20 kHz) | PWM pin of MOSFET module L | **10 kΩ pull-down to GND** | wheel off during boot. Duty capped by `WHEEL_MAX_DUTY` |
| 13 | J1-19 | `WHEEL_R_PWM` | out (PWM) | PWM pin of MOSFET module R | **10 kΩ pull-down to GND** | |
| 14 | J1-20 | *(spare)* | – | – | – | the MOSFET modules have no enable pin. Reserved for a future relay on the aim & feed rail |
| 1 | J3-4 | `FEED_HOME` | in | optical endstop SIGNAL (the feeder limiter of the prototype) | none: the board drives the line | `FEED_HOME_ACTIVE_LEVEL = LOW`. Board powered from 3V3, see `06-optical-endstop.md` |
| 2 | J3-5 | `LIMIT_PAN` | in | pan endstop, **only if one is added later** | internal `INPUT_PULLUP` | prototype taught pan/tilt limits by button; the app will do the same |
| 42 | J3-6 | `LIMIT_TILT` | in | tilt endstop, only if added later | internal `INPUT_PULLUP` | |
| 41 | J3-7 | `ESTOP_IN` | in | auxiliary contact on the wheel-rail E-STOP (optional) | internal `INPUT_PULLUP` | lets the app show "wheel power off" |
| 40 | J3-8 | `SHUTTLE_SENSE` | in | shuttle-present sensor (optional, future) | internal `INPUT_PULLUP` | |
| 48 | J3-16 | `RGB_LED` | out | onboard RGB LED | – | status: booting / AP up / client connected / e-stop. GPIO 38 on some boards |

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
| `5V` | Board supply input from the MEAN WELL N7805 (24 V → 5 V) when not on USB |
| `GND` (any) | Logic ground to the drivers' logic-GND pins, the MOSFET module header GNDs, the endstop GND, and one wire to the supply's star point |

## Microstepping jumpers (not GPIOs)

The Uno code drove M0/M1 from one GPIO. On the ESP32 they are jumpered to 3V3 per driver
and the multiplier is a constant per axis in `config.h`:

| Driver | Prototype setting | Proposed | M0 | M1 | M2 |
|---|---|---|---|---|---|
| Feeder | 1/8 (M0+M1 HIGH) | 1/8, keep | 3V3 | 3V3 | open |
| Pan | software ×2, hardware unclear | 1/16 | open | open | 3V3 |
| Tilt | software ×2, hardware unclear | 1/16 | open | open | 3V3 |

## Firmware constants tied to this map

| Constant | Meaning | Where it comes from |
|---|---|---|
| `WHEEL_MAX_DUTY` | Hard duty cap for GPIO 12/13, ≈ 50 % while the wheel rail is 24 V, 100 % on a 12 V rail | `04-launch-wheels-dc-motors.md` |
| `WHEEL_PWM_HZ` | LEDC frequency for the MOSFET modules, ≈ 16 kHz, ≤ 20 kHz (Uno used 980 Hz) | module spec |
| `FEED_HOME_ACTIVE_LEVEL` | Level on GPIO 1 that means "feeder at home": LOW on the prototype | `06-optical-endstop.md` |
| `SERVO_ARMS_CLOSED/OPEN`, `SERVO_SPOON_CLOSED/GUIDE/OPEN` | 10/40 and 180/105/0 degrees with `attach(pin, 544, 2400)` | `controller/reference-uno/README.md` |

## Why these pins

- GPIO 4–7 and 15–18 are eight consecutive header pins on J1: all stepper signals on one
  ribbon cable.
- Servo and wheel PWM on 10–13, the next block down on J1.
- Inputs on J3 (1, 2, 42, 41, 40) near the top of the right column, away from the power pins.
- Nothing on strapping pins, USB pins, UART0 pins or PSRAM pins.
- GPIO 1 and 2 are ADC1 pins, so a homing switch can later be replaced by a potentiometer
  or Hall sensor without rewiring.
