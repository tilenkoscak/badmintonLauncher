# Parts list (electronics)

Every electronic part that is, or will be, in the shuttle launcher. Mechanical parts
(frame, disks, shuttle magazine) are out of scope here.

Notes to Claude are marked `❓ **OPEN:**`. Each one says what must be confirmed with Tilen
before the wiring or firmware that depends on that part can be finalized. A consolidated
checklist is at the bottom of this file.

## Summary

| # | Part | Qty | Used for | What we know |
|---|---|---|---|---|
| 1 | ESP32-S3 dev board, N16R8, 44 pins, USB-C | 1 | Main controller: Wi-Fi access point, web server, all motion control | Module variant known; exact board details to verify |
| 2 | DRV8825 stepper driver carrier | 3 | Pan, tilt and feeder steppers | Well-known part; clone details to verify |
| 3 | Stepper motor | 3 | Pan, tilt, feeder | **Model and ratings unknown** |
| 4 | Hobby servo | 2 | Feeder mechanism | **Model unknown** |
| 5 | 775 brushed DC motor, 12 V, 150 W, 10 000 rpm | 2 | Launch wheels | Known from the Amazon listing |
| 6 | PWM power stage / motor driver for the 775 motors | ? | Speed control of the launch wheels | **Unknown: type, rating, logic level** |
| 7 | External power supply | 1 | Everything | **Voltage and current unknown** |
| 8 | Arduino Uno | 1 | Prototype controller, being replaced | Known; kept as reference |
| 9 | "Other electronic components" used in the prototype | ? | Capacitors, regulators, wiring… | **Unknown** |

---

## 1. ESP32-S3 development board (N16R8, 44 pin)

AliExpress listing: *ESP32-S3 Wifi BT Module Development Board for Arduino IDE ESP32-S3
N16R8 N8R2 44Pin Type-C 16MB Flash 8M PSRAM ESP32 S3*. Tilen has the **N16R8** variant.

This is a clone of the Espressif **ESP32-S3-DevKitC-1** carrying an
**ESP32-S3-WROOM-1-N16R8** module.

| Property | Value |
|---|---|
| CPU | Dual-core Xtensa LX7, up to 240 MHz |
| Radio | Wi-Fi 2.4 GHz (802.11 b/g/n) + Bluetooth 5 LE |
| Flash | 16 MB (N16) |
| PSRAM | 8 MB **octal** (R8). This is what makes GPIO 35/36/37 unusable |
| Logic level | **3.3 V, not 5 V tolerant** |
| GPIO current | About 20 mA per pin is a safe design value (absolute max ~40 mA) |
| Header | 2 × 22 pins, usually labelled with GPIO numbers on the silkscreen |
| Onboard | RST + BOOT buttons, addressable RGB LED (GPIO 48 on most clones, GPIO 38 on some), 3.3 V LDO regulator |
| USB | Type-C. DevKitC-1 style boards have **two** USB-C ports: "COM"/"UART" (through a USB-serial chip) and "USB" (native USB on GPIO 19/20) |
| Power in | USB-C 5 V, or the `5V` header pin, or regulated 3.3 V into `3V3` (not recommended) |

Role in this project: runs as a Wi-Fi access point, serves the web app to the phone,
receives commands over WebSocket, generates STEP/DIR for three DRV8825s, PWM for two servos
and PWM for two launch-wheel drivers. Details in `wiring/01-esp32-s3-basics.md`.

> ❓ **OPEN:** Before I finalize the board section of the wiring docs I need a photo of the
> top side of the board (or the pin labels read out) so I can confirm: (a) the header order
> matches the official DevKitC-1 v1.1 layout that `wiring/pin-map.md` assumes, (b) whether the
> board has one or two USB-C ports and how they are labelled, (c) which USB-serial chip it has
> (CH343 / CH340 / CP2102, matters for the Windows driver), (d) whether the RGB LED is on
> GPIO 48 or 38.

> ❓ **OPEN:** I need to know whether the module has the onboard PCB antenna (WROOM-1) or a
> u.FL connector for an external antenna (WROOM-1U). If it is the 1U variant an antenna
> must be attached before Wi-Fi is enabled.

## 2. DRV8825 stepper motor driver carrier (×3)

The classic 16-pin carrier (Pololu layout, also every AliExpress clone).

| Property | Value |
|---|---|
| Motor supply (VMOT) | 8.2 – 45 V |
| Output current | 1.5 A/phase without extra cooling, up to 2.2 A/phase with heatsink + airflow |
| Microstepping | full, 1/2, 1/4, 1/8, 1/16, 1/32 via M0/M1/M2 |
| Logic inputs | STEP, DIR, ENABLE, RESET, SLEEP, M0–M2. Logic high threshold **2.2 V ⇒ 3.3 V from the ESP32 is fine** |
| FAULT output | Open-drain, active low |
| Current limit | Set with the onboard potentiometer. `I_limit = Vref / (5 × R_sense)`; with the usual 0.100 Ω sense resistors that is **I = 2 × Vref** |
| Needs | ≥ 100 µF electrolytic across VMOT/GND, close to the board |

One driver each for pan, tilt and feeder. Details in `wiring/02-steppers-drv8825.md`.

> ❓ **OPEN:** Before I write the current-limit procedure I need to know whether these are
> genuine Pololu boards or clones, and the marking on the two small sense resistors next to
> the chip (`R100` = 0.1 Ω → I = 2·Vref; `R200` = 0.2 Ω → I = 1·Vref). Also: do they have
> heatsinks fitted, and what Vref / microstep jumper setting was used on the Uno prototype for
> each of the three drivers?

## 3. Stepper motors (×3: pan, tilt, feeder)

Nothing is known about these yet except that they run from DRV8825 drivers, so they are
bipolar (4-wire) or 6/8-wire motors wired as bipolar.

> ❓ **OPEN:** Before I can define the DRV8825 current limits, the VMOT voltage and the
> steps-per-degree maths for pan and tilt, I need for **each** stepper: the label on the motor
> (e.g. `17HS4401`), or failing that: rated current per phase, phase resistance or voltage,
> step angle (1.8° or 0.9°), number of wires, and NEMA size. I also need to know whether pan
> and tilt drive the axis directly or through a gear/belt reduction (ratio), and what the
> feeder stepper physically does (indexes a carousel? turns a screw?).

## 4. Hobby servos (×2, feeder)

Standard 3-wire hobby servos (GND, V+, signal). Signal is a 50 Hz pulse of 1–2 ms.
The ESP32's 3.3 V signal is accepted by practically all analog and digital hobby servos.

> ❓ **OPEN:** Before I can define the servo power rail I need the servo model
> (e.g. SG90, MG90S, MG996R, DS3218…) and how they were powered on the Uno prototype
> (from the Uno's 5 V pin? separate supply?). Metal-gear servos such as MG996R stall at
> ~2.5 A each and must not be powered from any microcontroller board.

> ❓ **OPEN:** Before I design the feed sequence in firmware I need a description of what each
> servo does and in what order servo A, servo B and the feeder stepper move for one shuttle,
> including the angles/positions and delays that worked on the prototype.

## 5. 775 brushed DC motors, 12 V, 150 W (×2, launch wheels)

Amazon.de listing: *Esportsmjj 775 Motor DC 12 V 10000RPM Motor Double Ball Bearings High
Power 150 W High Torque Motor*.

| Property | Value |
|---|---|
| Nominal voltage | 12 V |
| No-load speed | 10 000 rpm |
| Rated power (listing) | 150 W ⇒ roughly **12.5 A at rated load** |
| No-load current | Not stated; typically 1–2 A for this motor class |
| Stall current | Not stated; **tens of amps** is normal for a 775. Must be assumed when sizing the driver, fuse and wiring |
| Shaft | 5 mm (typical 775) |

One motor per launch disk, disks counter-rotate. Speed is set by PWM duty cycle.
Details in `wiring/04-launch-wheels-dc-motors.md`.

> ❓ **OPEN:** Before I write the wheel wiring I need to know how the two motors were made to
> spin in opposite directions on the prototype (swapped wires, or a reversible driver) and
> whether both wheels must be adjustable independently (needed for spin/slice shots) or
> always run at the same speed.

## 6. PWM power stage / driver for the 775 motors

Tilen wrote that the disks are "controlled by PWM", so some MOSFET or H-bridge stage sits
between the Uno and the motors. Its identity is the single most important unknown for the
wiring, because it decides the driver's logic level (many cheap modules do not switch fully
at 3.3 V), the current rating, and whether direction can be reversed.

> ❓ **OPEN:** Before I can draw the launch-wheel wiring I need the exact name or a photo of
> the module(s) driving the 775 motors on the prototype (e.g. "BTS7960 / IBT-2",
> "D4184 MOSFET module", "IRF520 module", an ESC, a relay…), how many there are, and what
> PWM frequency the Uno used. If it is an IRF520/IRF540-based module it will not work
> reliably from 3.3 V and I will propose a replacement.

## 7. External power supply

The prototype ran from an external supply of unknown rating.

> ❓ **OPEN:** Before I write `wiring/05-power.md` properly I need: the supply's output
> voltage(s) and current rating (label photo is ideal), what type it is (bench PSU, laptop
> brick, LED-strip PSU, battery…), and whether the finished robot must be battery-powered /
> portable on court or can be plugged into mains. Rough budget so far: two 775 motors can
> peak above 25 A together, so a 12 V / 30 A class supply or a 3S LiPo/Li-ion pack is the
> likely answer.

## 8. Arduino Uno (prototype controller)

Kept only as a reference. Everything the Uno sketches encode (step rates, accelerations,
servo angles, feed timing, PWM duty for a given launch distance) is directly reusable in
the ESP32 firmware. Note the Uno is 5 V logic; none of its 5 V wiring may be reused on
ESP32 inputs.

> ❓ **OPEN:** Before I write the firmware I need the Uno sketches from the prototype. Please
> copy them into `controller/reference-uno/` (any state, they do not need to be clean).

## 9. Other electronic components from the prototype

Tilen mentioned "other electronic components needed for the thing to work" without listing them.

> ❓ **OPEN:** Before I finalize any wiring doc I need the list of remaining parts: capacitors
> (values, where), voltage regulators / buck converters, breadboards or perfboard, switches,
> connectors, level shifters, diodes, fuses, limit switches, anything else on the bench.

---

## Parts probably still needed (suggestions, not yet confirmed)

These are not in the prototype description but the ESP32 build will very likely need them.
To be confirmed once the open questions above are answered.

| Part | Why |
|---|---|
| 5 V buck converter, ≥ 3 A (from the 12 V rail) | Powers the servos. Must not come from the ESP32 board |
| Small 5 V buck (≥ 1 A), or the same buck with good decoupling | Powers the ESP32 via its `5V` pin when not on USB. A separate buck avoids brown-out resets when a servo stalls |
| 3 × 100 µF (≥ 35 V) electrolytic capacitors | One per DRV8825 across VMOT/GND, if not already present |
| 470–1000 µF capacitor on the servo rail | Absorbs servo stall spikes |
| 2 × limit / micro switches | Homing of pan and tilt so the robot knows where it points after power-up |
| Physical E-stop or power switch on the 12 V motor rail | Cuts the wheels and steppers without killing the ESP32, so the app stays connected |
| Fuses on the 12 V rail (one per 775 motor) | Protection for the high-current branch |
| Wire ≥ 1.5 mm² (16 AWG) for the 775 motors | 12 A continuous, much more at stall |
| Optional: shuttle-present sensor (IR reflective) | Lets firmware confirm a shuttle actually fed |
| Optional: RPM sensor per wheel | Closed-loop wheel speed = repeatable launch distance |

## Open questions checklist

Short version of every `❓ OPEN:` above, to answer in one go:

1. Photo / pin labels of the ESP32-S3 board; one or two USB-C ports; RGB LED pin; USB-serial chip; PCB antenna or u.FL.
2. DRV8825: Pololu or clone, sense resistor marking, heatsinks, Vref and microstep settings used per driver.
3. Stepper motors: label or ratings for each of the three; gear/belt ratios on pan & tilt; what the feeder stepper does.
4. Servos: model, how they were powered, what each does in the feed sequence (angles, order, delays).
5. 775 motors: how counter-rotation is achieved; same speed or independent per wheel.
6. Driver/PWM stage for the 775 motors: exact module name or photo, quantity, PWM frequency used.
7. Power supply: voltage, current, type; battery/portable requirement.
8. Uno prototype sketches → `controller/reference-uno/`.
9. List of all other electronic components on the bench.
