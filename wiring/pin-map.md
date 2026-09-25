# GPIO pin map

**Draft v0.2 (2026-09-25).** Every signal between the ESP32-S3 and the rest of the machine.
The firmware's pin definitions and `PARTS.md` must match this table; change all three together.

Header positions refer to the DevKitC-1 v1.1 layout drawn in `01-esp32-s3-basics.md`
(J1 = left column, J3 = right column, counted from the USB end). Confirm against the
silkscreen before soldering.

> ❓ **OPEN:** The stepper, servo and wheel-PWM pins can be considered final once Tilen
> confirms the board's header layout. The input pins (1, 2, 42, 41) stay tentative until the
> optical endstop's role is decided (`PARTS.md` #13).

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
| 10 | J1-16 | `SERVO_A` | out (PWM 50 Hz) | feeder servo A signal (Miuzei 15 kg) | – | |
| 11 | J1-17 | `SERVO_B` | out (PWM 50 Hz) | feeder servo B signal | – | |
| 12 | J1-18 | `WHEEL_L_PWM` | out (PWM, `WHEEL_PWM_HZ` ≤ 20 kHz) | PWM pin of MOSFET module L | **10 kΩ pull-down to GND** | wheel off during boot. Duty capped by `WHEEL_MAX_DUTY` |
| 13 | J1-19 | `WHEEL_R_PWM` | out (PWM) | PWM pin of MOSFET module R | **10 kΩ pull-down to GND** | |
| 14 | J1-20 | *(spare)* | – | – | – | was `WHEEL_EN`; the MOSFET modules have no enable pin. Reserved for a future relay on the aim & feed rail |
| 1 | J3-4 | `LIMIT_PAN` | in | optical endstop SIGNAL or a micro switch to GND | internal `INPUT_PULLUP` | level set by `LIMIT_ACTIVE_LEVEL`. ADC1-capable if a pot is ever preferred |
| 2 | J3-5 | `LIMIT_TILT` | in | second endstop | internal `INPUT_PULLUP` | |
| 42 | J3-6 | `SHUTTLE_SENSE` | in | shuttle-present sensor (optional; the optical endstop could take this role instead) | internal `INPUT_PULLUP` | |
| 41 | J3-7 | `ESTOP_IN` | in | auxiliary contact on the wheel-rail E-STOP (optional) | internal `INPUT_PULLUP` | lets the app show "wheel power off" |
| 48 | J3-16 | `RGB_LED` | out | onboard RGB LED | – | status: booting / AP up / client connected / e-stop. GPIO 38 on some boards |

Spare, free for later: **14, 21, 38, 39, 40, 47** (38 only if the LED is not on it).

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
| `GND` (any) | Logic ground to the drivers' logic-GND pins, the MOSFET module header GNDs, and one wire to the supply's star point |

## Firmware constants tied to this map

| Constant | Meaning | Where it comes from |
|---|---|---|
| `WHEEL_MAX_DUTY` | Hard duty cap for GPIO 12/13, ≈ 50 % while the wheel rail is 24 V | `04-launch-wheels-dc-motors.md` |
| `WHEEL_PWM_HZ` | LEDC frequency for the MOSFET modules, 1 kHz to start, ≤ 20 kHz | module spec |
| `LIMIT_ACTIVE_LEVEL` | Which level on GPIO 1/2 means "endstop reached" | smoke test in `06-optical-endstop.md` |

## Why these pins

- GPIO 4–7 and 15–18 are eight consecutive header pins on J1: all stepper signals on one
  ribbon cable.
- Servo and wheel PWM on 10–13, the next block down on J1.
- Inputs on J3 (1, 2, 42, 41) near the top of the right column, away from the power pins.
- Nothing on strapping pins, USB pins, UART0 pins or PSRAM pins.
- GPIO 1 and 2 are ADC1 pins, so a homing switch can later be replaced by a potentiometer
  or Hall sensor without rewiring.
