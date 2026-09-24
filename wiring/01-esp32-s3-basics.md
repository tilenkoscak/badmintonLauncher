# 01 · The ESP32-S3 board (N16R8, 44 pin)

What the board is, which pins are safe to use, how to power it and how to set up the
Arduino IDE for it. Written for someone who knows the Arduino Uno and is holding an ESP32
for the first time.

## Uno → ESP32-S3: what changes

| Topic | Arduino Uno | ESP32-S3 (this board) |
|---|---|---|
| Logic level | 5 V | **3.3 V**. Pins are **not** 5 V tolerant |
| Pin names | `D2`…`D13`, `A0`… | Plain GPIO numbers: `4`, `17`, `42`… as printed on the board |
| Forbidden pins | 0/1 (serial) | Several, see the table below. Most important: **35, 36, 37** (PSRAM), 19/20 (USB), 43/44 (serial) |
| PWM | 6 fixed pins, `analogWrite()` | Any GPIO, 8 hardware PWM channels (LEDC). `analogWrite()` exists, `ledcWrite()` gives full control of frequency and resolution |
| Servos | `Servo.h` | `ESP32Servo.h` (the standard Servo library does not support ESP32) |
| Analog in | 6 × 10-bit | Many × 12-bit, but ADC2 pins (GPIO 11–20) are **unusable while Wi-Fi is on**. Use ADC1 (GPIO 1–10) |
| Speed / RAM | 16 MHz, 2 KB | 2 × 240 MHz, 512 KB SRAM + 8 MB PSRAM, 16 MB flash |
| Connectivity | none | Wi-Fi + Bluetooth LE on the module |
| Cores | 1 | 2. In Arduino the Wi-Fi stack runs on core 0, `setup()`/`loop()` on core 1 |
| Current per pin | 20–40 mA | ≈ 20 mA safe, 40 mA absolute max |
| 3.3 V output | 50 mA from the Uno's regulator | Board LDO is typically 500–800 mA **including** the ESP32 itself, which peaks at ~500 mA during Wi-Fi. Do not power anything but small sensors from `3V3` |
| Serial monitor | one USB port | Two USB-C ports on most of these boards, with different behaviour (see below) |
| Boot | reset button | Strapping pins (0, 3, 45, 46) decide the boot mode; keep them free |

## Board anatomy (typical DevKitC-1 clone)

```
                         ┌────── USB-C "COM/UART" ──┐  ┌── USB-C "USB" ──┐
                         │  via CH343/CP2102 → GPIO43/44 │ native USB → GPIO19/20 │
                         └───────────────────────────────┴─────────────────────────┘
   J1 (left)                                                          J3 (right)
   3V3  ── ●                                                           ● ── GND
   3V3  ── ●                                                           ● ── TX  (GPIO 43)  ✗ serial
   RST  ── ●                                                           ● ── RX  (GPIO 44)  ✗ serial
   GPIO 4  ── ●                                                        ● ── GPIO 1
   GPIO 5  ── ●                                                        ● ── GPIO 2
   GPIO 6  ── ●                                                        ● ── GPIO 42
   GPIO 7  ── ●                                                        ● ── GPIO 41
   GPIO 15 ── ●                                                        ● ── GPIO 40
   GPIO 16 ── ●                                                        ● ── GPIO 39
   GPIO 17 ── ●                                                        ● ── GPIO 38  (RGB LED on some boards)
   GPIO 18 ── ●                                                        ● ── GPIO 37  ✗ PSRAM
   GPIO 8  ── ●                                                        ● ── GPIO 36  ✗ PSRAM
   GPIO 3  ── ●  strapping                                             ● ── GPIO 35  ✗ PSRAM
   GPIO 46 ── ●  strapping                                             ● ── GPIO 0   strapping (BOOT button)
   GPIO 9  ── ●                                                        ● ── GPIO 45  strapping
   GPIO 10 ── ●                                                        ● ── GPIO 48  (RGB LED on most boards)
   GPIO 11 ── ●                                                        ● ── GPIO 47
   GPIO 12 ── ●                                                        ● ── GPIO 21
   GPIO 13 ── ●                                                        ● ── GPIO 20  ✗ USB D+
   GPIO 14 ── ●                                                        ● ── GPIO 19  ✗ USB D−
   5V   ── ●                                                           ● ── GND
   GND  ── ●                                                           ● ── GND
                          [BOOT]                      [RST]           [RGB LED]
```

This is the official Espressif ESP32-S3-DevKitC-1 v1.1 layout, which the 44-pin AliExpress
clones normally copy. **Trust the silkscreen on your board over this drawing.**

> ❓ **OPEN:** Before I mark this drawing as verified I need a photo of Tilen's board or the
> pin labels read out row by row, and confirmation of how the USB-C port(s) are labelled.

## Pins: forbidden, careful, free

| Category | GPIOs | Why |
|---|---|---|
| **Never use** | 35, 36, 37 | Wired to the 8 MB octal PSRAM inside the module. Using them corrupts memory or crashes the chip. This is specific to the R8 variant |
| **Never use** | 19, 20 | Native USB D−/D+. Needed for the "USB" port, USB CDC serial and flashing over native USB |
| **Never use** | 43, 44 | UART0 TX/RX, wired to the USB-serial chip of the "COM" port. Needed for uploads and the serial monitor |
| **Avoid** | 0, 3, 45, 46 | Strapping pins: their level at reset selects boot mode / flash voltage / logging. A motor driver pulling one of them at power-up can stop the board from booting |
| **Check first** | 48, 38 | One of them drives the onboard RGB LED. Fine as a status LED, do not use for motors |
| **Keep free if possible** | 8, 9 | Default I²C SDA/SCL in the Arduino core. Useful later for a display or IMU |
| **Fine, but** | 39, 40, 41, 42 | Default JTAG pins. Only matters if you ever debug with a JTAG probe. Free to use as GPIO |
| **Free** | 1, 2, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 47 | General purpose. Any of them can do STEP/DIR, PWM or inputs |

That leaves 21 comfortable GPIOs plus 39–42, more than the ~17 this project needs.
The actual assignment is in `pin-map.md`.

Every GPIO is input/output-capable, can generate PWM and can trigger interrupts. There is no
"PWM-only-on-these-pins" rule as on the Uno.

## Powering the board

| Situation | How |
|---|---|
| On the bench, developing | USB-C from the PC. Nothing else needed. Connect **only GND** between the board and the motor supply, plus the signal wires |
| In the robot | 5 V from a buck converter into the `5V` pin and `GND`. A dedicated small buck (≥ 1 A) for the ESP32 is better than sharing the servo buck: a stalling servo pulls the 5 V rail down and the ESP32 brown-out detector resets the board |
| Never | Feed 5 V or 12 V into `3V3` or into any GPIO |

Rules of thumb:

- The `3V3` pin is an **output** of the board's regulator. Use it only for DRV8825
  RESET/SLEEP, pull-up resistors and small sensors.
- `5V` pin and USB at the same time: DevKitC-1 style boards have a diode between USB and the
  `5V` rail so both can be connected, but not every clone has it. Verify with a multimeter
  (no continuity from `5V` pin back to the USB connector's VBUS) before ever plugging in
  the PC while the buck is on. If unsure, unplug the buck while developing over USB.
- The board has a brown-out detector. Random resets when motors start = the 5 V supply
  to the board is sagging. Fix the power, do not disable the detector.

## Arduino IDE setup

1. Install Arduino IDE 2.x.
2. *Boards Manager* → search **esp32** → install **"esp32 by Espressif Systems"** (3.x).
   If it does not show up, add this URL under *File → Preferences → Additional boards manager
   URLs* and search again:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Plug the board into the PC using the USB-C port labelled **COM** / **UART** (the one
   going through the USB-serial chip). This is the most reliable port for uploads.
   Windows 11 normally installs the CH343 / CP2102 driver by itself; if no COM port appears,
   the chip's driver has to be installed manually.
4. *Tools* menu settings for the N16R8 board:

   | Setting | Value |
   |---|---|
   | Board | **ESP32S3 Dev Module** |
   | Port | the COM port that appeared when plugging in |
   | USB CDC On Boot | **Disabled** when using the COM/UART port. (**Enabled** if you use the native "USB" port for the serial monitor) |
   | CPU Frequency | 240 MHz (WiFi) |
   | Flash Mode | QIO 80 MHz |
   | Flash Size | **16MB (128Mb)** |
   | Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
   | PSRAM | **OPI PSRAM** (choosing "QSPI PSRAM" on an R8 board boot-loops with a PSRAM error) |
   | Upload Speed | 921600 |
   | Upload Mode | UART0 / Hardware CDC |
   | USB Mode | Hardware CDC and JTAG |
   | Core Debug Level | None |

5. If an upload fails with "Failed to connect", hold **BOOT**, tap **RST**, release BOOT,
   then upload again.

About `Serial`: with *USB CDC On Boot = Disabled*, `Serial` is UART0 and appears on the COM
port. With it *Enabled*, `Serial` is the native USB port and UART0 becomes `Serial0`.
Pick one setup and stick with it.

## Smoke test 1: LED + serial

```cpp
// Blink the onboard RGB LED and print. Board: ESP32S3 Dev Module, arduino-esp32 core 3.x
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48   // try 38 if nothing lights up
#endif

void setup() {
  Serial.begin(115200);
}

void loop() {
  rgbLedWrite(RGB_BUILTIN, 16, 0, 0);  delay(400);   // dim red   (core 2.x: neopixelWrite)
  rgbLedWrite(RGB_BUILTIN, 0, 16, 0);  delay(400);   // dim green
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);   delay(400);
  Serial.printf("ESP32-S3 alive, free heap %u, PSRAM %u\n", ESP.getFreeHeap(), ESP.getPsramSize());
}
```

Expected: LED cycles red/green/off, serial monitor at 115200 baud prints a line every
1.2 s with PSRAM ≈ 8 388 608. If PSRAM prints 0, the *PSRAM* setting is wrong.

## Smoke test 2: Wi-Fi

*File → Examples → WiFi → WiFiScan*. Upload, open the serial monitor. Your home network must
appear in the list. This proves the radio and antenna work before any web-server code exists.

## Common pitfalls

- **Motor moves at power-up, then behaves.** A driver input was floating during boot.
  Add the pull-up / pull-down resistors listed in `pin-map.md`.
- **Board keeps resetting when a motor starts.** Brown-out. Power the ESP32 from its own
  regulator, add capacitance, check for a shared thin ground wire.
- **Board does not boot with drivers connected.** Something is pulling a strapping pin
  (0, 3, 45, 46) at reset. Move that signal.
- **Wi-Fi range is bad.** Motor wires or a metal frame are next to the antenna end of the
  module. Move the board or use a 1U module with an external antenna.
- **`analogRead` returns nonsense.** The pin is on ADC2 (GPIO 11–20) while Wi-Fi is active.
  Use GPIO 1–10.
