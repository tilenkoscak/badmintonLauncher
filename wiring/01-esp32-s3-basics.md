# 01 · The ESP32-S3 board (N16R8, 44 pin)

What the board is, which pins are safe to use, how to power it and how to set up the
PlatformIO for it. Written for someone who knows the Arduino Uno and is holding an ESP32
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
| Serial monitor | one USB port | Two USB-C ports on this board, with different behaviour (see below) |
| Boot | reset button | Strapping pins (0, 3, 45, 46) decide the boot mode; keep them free |

## Board anatomy (Tilen's board, verified from a photo)

Held with the components facing you, the antenna up and the USB-C ports down. Pin 1 of each
row is at the antenna end. `wiring-diagrams.html` has the same view in colour, with every
pin's assignment.

```
                    ┌─────────────────────────────┐
                    │  PCB antenna (overhangs the │   keep metal and motor wires away
                    │  board edge)                │
                    │  ESP32-S3-WROOM-1  N16R8    │
   J1 (left)        └─────────────────────────────┘                 J3 (right)
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
   5Vin ── ●                                                           ● ── GND
   GND  ── ●                                                           ● ── GND
              ┌──────────┐                                ┌──────────┐
              │  USB-C   │  (J1 side)          (J3 side)  │  USB-C   │  USB-serial chip next to it
              └──────────┘                                └──────────┘
   RGB LED beside J1 near GPIO 10/11.  BOOT and RET (= reset) buttons beside J3 near GPIO 35-37.
```

This is the official Espressif ESP32-S3-DevKitC-1 v1.1 layout. Tilen's board matches it
label for label (photo, 2026-10-20); the only differences are the names `5Vin` (official:
`5V`) and `RET` on the reset button. The module is an ESP32-S3-WROOM-1 N16R8 with a PCB
antenna.

### Which USB-C port is which

The ports are not labelled on the top side. The small USB-serial chip sits next to the port
on the J3 side, so that one is most likely **COM** (through the chip to UART0, GPIO 43/44) and
the one on the J1 side the **native USB** port (GPIO 19/20). To be sure, plug in one port at a
time and look at Windows Device Manager: the native port adds a **USB JTAG/serial debug unit**
device next to its COM port, the COM port does not. Smoke test 1 confirms it too: its output
appears only on the COM port. Use COM for uploads and the serial monitor.

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
| In the robot | 5 V from a buck converter into the `5Vin` pin and `GND`. A dedicated small buck (≥ 1 A) for the ESP32 is better than sharing the servo buck: a stalling servo pulls the 5 V rail down and the ESP32 brown-out detector resets the board |
| Never | Feed 5 V or 12 V into `3V3` or into any GPIO |

Rules of thumb:

- The `3V3` pin is an **output** of the board's regulator. Use it only for DRV8825
  RESET/SLEEP, pull-up resistors and small sensors.
- `5Vin` pin and USB at the same time: DevKitC-1 style boards have a diode between USB and the
  `5V` rail so both can be connected, but not every clone has it. Verify with a multimeter
  (no continuity from the `5Vin` pin back to the USB connector's VBUS) before ever plugging in
  the PC while the buck is on. If unsure, unplug the buck while developing over USB.
- The board has a brown-out detector. Random resets when motors start = the 5 V supply
  to the board is sagging. Fix the power, do not disable the detector.

## PlatformIO setup

All ESP32 code in this project is built with **PlatformIO**, like the Uno project in
`controller/reference-uno/ShuttlecockLauncher/`.

1. VS Code or Cursor with the **PlatformIO IDE** extension (already installed for the Uno
   project).
2. Plug the board into the PC using the **COM** port (the one going through the USB-serial
   chip; see "Which USB-C port is which" above). This is the most reliable port for uploads.
   Windows 11 normally installs a driver for the chip by itself; if no COM port appears,
   install WCH's CH343 driver. PlatformIO finds the port on its own.
3. `platformio.ini` for this board (N16R8: 16 MB QIO flash, 8 MB octal PSRAM):

   ```ini
   [env:esp32s3]
   ; pioarduino: community build of the Espressif platform with arduino-esp32 core 3.x
   platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
   board = esp32-s3-devkitc-1
   framework = arduino

   board_build.arduino.memory_type = qio_opi    ; QIO flash + OPI (octal) PSRAM = N16R8
   board_build.flash_mode = qio
   board_upload.flash_size = 16MB
   board_upload.maximum_size = 16777216
   board_build.partitions = default_16MB.csv
   build_flags =
       -DBOARD_HAS_PSRAM

   monitor_speed = 115200
   monitor_echo = yes                           ; show what you type in the serial monitor
   lib_deps =
       madhephaestus/ESP32Servo
   ```

4. If an upload fails with "Failed to connect", hold **BOOT**, tap **RST**, release BOOT,
   then upload again.

Why the pioarduino platform: PlatformIO's official `espressif32` platform has long shipped
arduino-esp32 core 2.x, while the code in these docs uses core 3.x calls (`ledcAttach`,
`rgbLedWrite`). The code comments name the 2.x equivalents in case the official platform is
used. Smoke test 1 below proves the memory settings: if it reports 0 bytes of PSRAM, the
`memory_type` line is wrong for this board.

About `Serial`: by default `Serial` is UART0 and appears on the COM port. Adding
`-DARDUINO_USB_CDC_ON_BOOT=1` to `build_flags` moves `Serial` to the native "USB" port and
UART0 becomes `Serial0`. This project uses the COM port; pick one setup and stick with it.

Every smoke test in these docs is a complete `src/main.cpp` for a project with the config
above. Its first line is `#include <Arduino.h>`: the Arduino IDE adds that silently,
PlatformIO does not. Once `controller/firmware/` exists, each smoke test becomes its own
environment there, the way the Uno project did it with `build_src_filter`.

## Smoke test 1: LED + serial

```cpp
#include <Arduino.h>

// Blink the onboard RGB LED and print. arduino-esp32 core 3.x
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
1.2 s with PSRAM ≈ 8 388 608. If PSRAM prints 0, the `memory_type` line in `platformio.ini` is wrong.

## Smoke test 2: Wi-Fi

```cpp
#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
}

void loop() {
  int n = WiFi.scanNetworks();
  Serial.printf("%d networks\n", n);
  for (int i = 0; i < n; i++) Serial.printf("  %-32s %4d dBm\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  delay(5000);
}
```

Your home network must appear in the list every 5 s. This proves the radio and antenna work
before any web-server code exists.

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
