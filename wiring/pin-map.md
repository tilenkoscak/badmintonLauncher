# GPIO pin map

**Draft v0.1 (2026-09-22).** Every signal between the ESP32-S3 and the rest of the machine.
The firmware's pin definitions and `PARTS.md` must match this table; change all three together.

Header positions refer to the DevKitC-1 v1.1 layout drawn in `01-esp32-s3-basics.md`
(J1 = left column, J3 = right column, counted from the USB end). Confirm against the
silkscreen before soldering.

> ❓ **OPEN:** The assignments for the launch-wheel driver (GPIO 12/13/14) and the optional
> inputs are tentative until the driver module and the sensors are known. The stepper and
> servo pins can be considered final once Tilen confirms the board's header layout.

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
| 10 | J1-16 | `SERVO_A` | out (PWM 50 Hz) | feeder servo A signal | – | |
| 11 | J1-17 | `SERVO_B` | out (PWM 50 Hz) | feeder servo B signal | – | |
| 12 | J1-18 | `WHEEL_L_PWM` | out (PWM 20 kHz) | left wheel driver PWM input | **10 kΩ pull-down to GND** | wheel off during boot |
| 13 | J1-19 | `WHEEL_R_PWM` | out (PWM 20 kHz) | right wheel driver PWM input | **10 kΩ pull-down to GND** | |
| 14 | J1-20 | `WHEEL_EN` | out | wheel driver enable(s), if the driver has them | **10 kΩ pull-down to GND** | tentative, depends on driver |
| 1 | J3-4 | `LIMIT_PAN` | in | pan homing switch (suggested) | internal `INPUT_PULLUP` | switch to GND. ADC1-capable if a pot is preferred |
| 2 | J3-5 | `LIMIT_TILT` | in | tilt homing switch (suggested) | internal `INPUT_PULLUP` | |
| 42 | J3-6 | `SHUTTLE_SENSE` | in | shuttle-present sensor (optional) | internal `INPUT_PULLUP` | |
| 41 | J3-7 | `ESTOP_IN` | in | contact on the physical e-stop (optional) | internal `INPUT_PULLUP` | lets the app show "e-stop pressed" |
| 48 | J3-16 | `RGB_LED` | out | onboard RGB LED | – | status: booting / AP up / client connected / e-stop. GPIO 38 on some boards |

Spare, free for later: **21, 38, 39, 40, 47** (38 only if the LED is not on it).

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
| `3V3` | DRV8825 RESET + SLEEP (×3), pull-up for `STEP_EN`, M0–M2 jumpers. Nothing that draws real current |
| `5V` | Board supply input from the ESP32's own 5 V buck when not on USB |
| `GND` (any) | Logic ground to the drivers' logic-GND pins and to the supply's star point |

## Why these pins

- GPIO 4–7 and 15–18 are eight consecutive header pins on J1: all stepper signals on one
  ribbon cable.
- Servo and wheel PWM on 10–14, the next block down on J1.
- Inputs on J3 (1, 2, 42, 41) near the top of the right column, away from the power pins.
- Nothing on strapping pins, USB pins, UART0 pins or PSRAM pins.
- GPIO 1 and 2 are ADC1 pins, so a homing switch can later be replaced by a potentiometer
  or Hall sensor without rewiring.
