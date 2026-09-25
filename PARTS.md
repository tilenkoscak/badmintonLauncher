# Parts list (electronics)

Every electronic part that is, or will be, in the shuttle launcher. Mechanical parts
(frame, disks, shuttle magazine) are out of scope here.

Notes to Claude are marked `❓ **OPEN:**`. Each one says what must be confirmed with Tilen
before the wiring or firmware that depends on that part can be finalized. A consolidated
checklist is at the bottom of this file.

A lot of the electronics is salvaged from an **Artillery Sidewinder X3 Plus** 3D printer:
the three steppers, the dual-output power supply and the optical endstop.

## Summary

| # | Part | Qty | Used for | What we know |
|---|---|---|---|---|
| 1 | ESP32-S3 dev board, N16R8, 44 pins, USB-C | 1 | Main controller: Wi-Fi access point, web server, all motion | Module variant known; board details to verify |
| 2 | DRV8825 stepper driver carrier, 100 µF/50 V capacitor already fitted | 3 | Pan, tilt and feeder steppers | Known part; sense-resistor marking to verify |
| 3 | NEMA 17 stepper from the Sidewinder X3 Plus (2 × Z-axis type "42-35", 1 × from another axis) | 3 | Pan, tilt, feeder | Size known; rated current estimated 1.2–1.7 A, label to read |
| 4 | Miuzei 15 kg digital servo, metal gear, 180° | 2 | Feeder | 4.8–7.4 V, 0.5–2.5 ms pulse |
| 5 | 775 brushed DC motor, 12 V, 150 W, 10 000 rpm | 2 | Launch wheels | Known from listing. **Fed from a 24 V rail in the prototype, see #7** |
| 6 | Artillery X3 Plus power supply, two outputs: 24 V / 4.2 A and 36 V / 9.7 A | 1 | Everything | Ratings known; model label and output isolation to verify |
| 7 | Buck converter 36 V → 24 V, IP68 potted module | 1 | 24 V rail for the launch wheels | Family known; current variant to confirm |
| 8 | Fuji Electric E83-004 Schottky diode, 40 V, 60 A, TO-3P | 2 | Series blocking diode, one per wheel branch | Known |
| 9 | Dual-MOSFET PWM switch module, DC 5–36 V, 15 A (30 A peak), 400 W | 2 | Speed control of each launch wheel | Known; trigger 3.3–20 V, PWM 0–20 kHz |
| 10 | Electrolytic capacitor 1000 µF / 50 V | 2 | Buffer in each wheel branch | Position in the circuit to confirm |
| 11 | TVS diode 1.5KE30A | 2 | Clamp / flyback path across each 775 motor | Known |
| 12 | MEAN WELL N7805-1CW switching regulator, 5 V / 1 A | 1 | 24 V → 5 V for the servos | Known; **undersized for two 15 kg servos** |
| 13 | Optical endstop from the Sidewinder X3 Plus | 1 | Homing or shuttle detection | Model, pinout and role to confirm |
| 14 | Arduino Uno | 1 | Prototype controller, being replaced | Reference only |

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
and PWM for two launch-wheel MOSFET modules. Details in `wiring/01-esp32-s3-basics.md`.

> ❓ **OPEN:** Before I finalize the board section of the wiring docs I need a photo of the
> top side of the board (or the pin labels read out) so I can confirm: (a) the header order
> matches the official DevKitC-1 v1.1 layout that `wiring/pin-map.md` assumes, (b) whether the
> board has one or two USB-C ports and how they are labelled, (c) which USB-serial chip it has
> (CH343 / CH340 / CP2102, matters for the Windows driver), (d) whether the RGB LED is on
> GPIO 48 or 38, (e) whether the module has a PCB antenna (WROOM-1) or a u.FL connector for
> an external antenna (WROOM-1U).

## 2. DRV8825 stepper motor driver carrier (×3)

The classic 16-pin carrier (Pololu layout, also every AliExpress clone). Each one already
has a **100 µF / 50 V electrolytic** across VMOT/GND, which is exactly what the 24 V motor
rail needs; nothing to add there.

| Property | Value |
|---|---|
| Motor supply (VMOT) | 8.2 – 45 V. In this project: **24 V** (PSU output 1) |
| Output current | 1.5 A/phase without extra cooling, up to 2.2 A/phase with heatsink + airflow |
| Microstepping | full, 1/2, 1/4, 1/8, 1/16, 1/32 via M0/M1/M2 |
| Logic inputs | STEP, DIR, ENABLE, RESET, SLEEP, M0–M2. Logic high threshold **2.2 V ⇒ 3.3 V from the ESP32 is fine** |
| FAULT output | Open-drain, active low |
| Current limit | Set with the onboard potentiometer. `I_limit = Vref / (5 × R_sense)`; with the usual 0.100 Ω sense resistors that is **I = 2 × Vref** |

One driver each for pan, tilt and feeder. Details in `wiring/02-steppers-drv8825.md`.
The current limits tuned on the Uno prototype stay valid: they do not depend on the
microcontroller, only on the motor.

> ❓ **OPEN:** Before I write final Vref values I need the marking on the two small sense
> resistors next to the chip (`R100` = 0.1 Ω → I = 2·Vref; `R200` = 0.2 Ω → I = 1·Vref),
> whether heatsinks are fitted, and the Vref / microstep jumper settings currently on each of
> the three drivers (measure Vref as described in `wiring/02-steppers-drv8825.md`).

## 3. Stepper motors (×3: pan, tilt, feeder)

All three are salvaged from the **Artillery Sidewinder X3 Plus**. Two are the Z-axis motors
(the printer has dual Z), the third comes from another axis of the same printer and is
believed to be the same size and rating.

| Property | Value |
|---|---|
| Frame | NEMA 17 (42 mm). Artillery lists the X3/X4 Z-axis motor as type **"42-35"** (42 mm square, 35 mm long). The X-axis motor is a "42-40", the Y-axis a "42-48" |
| Step angle | 1.8° (200 full steps per revolution) is standard for these printers; confirm on the label |
| Rated current | **Estimated 1.2–1.7 A per phase** (Tilen). Typical for a 42-35 is 1.3–1.5 A |
| Wiring | Bipolar, 4 wires, Artillery motors come with a JST-XH plug (4-pin, or 6-pin with 4 populated) |

> ❓ **OPEN:** Before I fix the DRV8825 current limits and the steps-per-degree maths I need,
> for each motor, the text printed on its label (model number and current, e.g. "42HB34F...
> 1.3A" or "17HS15-1504S") and which motor sits on which axis (pan / tilt / feeder). If the
> third motor is not a 42-35, its label tells us. I also need the gear or belt ratio between
> each stepper and its axis, and what the feeder stepper physically does (indexes a carousel?
> turns a screw?).

## 4. Hobby servos (×2, feeder)

Amazon.de B0C4T73SMB: **Miuzei Digital Servo 15 kg, metal gear, 180°** (sold as a 5-pack).

| Property | Value (from the listing) |
|---|---|
| Operating voltage | **4.8 – 7.4 V** |
| Stall torque | 15 kg·cm (at the upper end of the voltage range; less at 5 V) |
| Speed | 0.13 s / 60° |
| Rotation | 180° |
| Control pulse | 0.5 – 2.5 ms, 50 Hz (digital servo) |
| Gears | Metal (copper alloy) |
| Cable | 320 mm, standard 3-wire (brown GND, red V+, orange signal) |
| Stall current | Not published. 15 kg-class digital servos typically draw 2–3 A each when stalled and around 1 A while moving under load |

Powered in the prototype from the 24 V rail through the MEAN WELL N7805 (#12) at **5 V**.

**Correction to the prototype notes:** the servos do *not* need 12 V. The listing says
4.8–7.4 V, and 12 V would destroy them. The N7805 outputs 5 V, which is right. If the
regulator on the bench is in fact an N78**12** (12 V), the servos are being over-volted.

> ❓ **OPEN:** Before I finalize `wiring/03-servos.md` I need Tilen to read the marking on
> the MEAN WELL regulator and confirm it is N78**05**-1CW (5 V output), then measure the
> voltage on the servo red wire while the prototype runs.

> ❓ **OPEN:** Before I design the feed sequence in firmware I need a description of what each
> servo does and in what order servo A, servo B and the feeder stepper move for one shuttle,
> including the angles and delays that worked on the prototype.

## 5. 775 brushed DC motors, 12 V, 150 W (×2, launch wheels)

Amazon.de listing: *Esportsmjj 775 Motor DC 12 V 10000RPM Motor Double Ball Bearings High
Power 150 W High Torque Motor*.

| Property | Value |
|---|---|
| Nominal voltage | **12 V** |
| No-load speed | 10 000 rpm at 12 V |
| Rated power (listing) | 150 W |
| No-load current | Not stated; typically 1–2 A for this motor class |
| Stall current | Not stated; **tens of amps** is normal for a 775 |
| Shaft | 5 mm (typical 775) |

One motor per launch disk, disks counter-rotate. Speed is set by PWM duty cycle through the
MOSFET modules (#9). Details in `wiring/04-launch-wheels-dc-motors.md`.

**Important:** in the prototype these 12 V motors hang on a **24 V rail** (buck converter #7).
At 100 % PWM duty they would see twice their rated voltage (≈ 20 000 rpm, brushes and
bearings far outside spec). The prototype presumably never ran high duty. The firmware
therefore gets a hard cap `WHEEL_MAX_DUTY` of about 50 % (12 V average), unless the buck is
swapped for a 12 V one. The MOSFET module switches one direction only, so the counter-rotation
must come from swapping the two motor leads on one wheel.

> ❓ **OPEN:** Before I fix `WHEEL_MAX_DUTY` I need to know the highest PWM value (0–255 on the
> Uno) the prototype used for a normal shot, and to have the buck's real output voltage
> measured with a multimeter. I also need to know whether the two wheels must be adjustable
> independently (spin shots) or always run at the same speed.

## 6. Power supply: Artillery Sidewinder X3 Plus PSU (dual output)

Salvaged from the printer. Mains input, two isolated-looking DC outputs:

| Output | Rating | Power | Used for (prototype) |
|---|---|---|---|
| 1 | **24 V / 4.2 A** | ≈ 100 W | Three DRV8825 (VMOT) and, via the N7805, the two servos |
| 2 | **36 V / 9.7 A** | ≈ 350 W | Buck converter (#7) → launch wheels |

Because the supply is a mains unit, the robot is treated as **mains-powered**; the battery
option from the first draft is dropped.

> ❓ **OPEN:** Before I finish `wiring/05-power.md` I need: the model printed on the PSU label,
> and one multimeter test: is there continuity between the negative terminal of output 1 and
> the negative terminal of output 2? (Decides whether the two grounds are already joined
> inside the PSU or must be joined at our star point.) Also whether the PSU has a mains switch
> and whether it came with its fan.

## 7. Buck converter 36 V → 24 V (launch-wheel rail)

AliExpress 1005006451305852: *DC DC 36V 48V to 24V 1A 2A 3A 5A 10A 15A 20A 30A Step Down
… 480W Buck Converter IP68 Waterproof*. A potted aluminium module with wire leads,
**fixed 24 V output**, sold in current variants from 1 A to 30 A; the 36/48 V version accepts
30–60 V input. Non-isolated (input − and output − are the same node) as is usual for this type.

Fed from PSU output 2 (36 V / 9.7 A ≈ 350 W), so whatever the variant says, the rail can
deliver at most about **13 A at 24 V** (≈ 90 % efficiency) before the PSU limits.

> ❓ **OPEN:** Before I size the fuses and the firmware soft-start I need to know which current
> variant was bought (printed on the module, e.g. "24V 10A"), the measured output voltage under
> load, and whether the module current-limits or simply shuts down when overloaded (matters
> when both wheels spin up at once).

## 8. Schottky diode Fuji Electric E83-004 (×2)

AliExpress 1005009025204329: *E83-004 TO-3P 60A 40V Schottky Rectifier Diode*. Fuji
Electric Schottky barrier diode: 40 V reverse, 60 A total as two 30 A elements in a
3-terminal TO-3P package (anode – common cathode – anode), forward drop about 0.55 V.

Position in the prototype: in **series** between the buck output and each MOSFET module.
Function: stops current flowing back into the buck converter when a spinning wheel is
slowed down or switched off (the motor then acts as a generator). Dissipates roughly
0.55 V × motor current, i.e. 3–4 W per diode at full wheel power: needs a heatsink. The tab
is the cathode, so isolate it if both diodes share one heatsink.

> ❓ **OPEN:** Before I draw the wheel-branch diagram I need to know whether one element or
> both paralleled elements of the TO-3P are used, and that the anode side faces the buck.

## 9. Dual-MOSFET PWM switch module, 15 A / 400 W (×2)

AliExpress 1005007594739156: *DC 5V-36V 15A Max 30A 400W Dual High-Power MOSFET Trigger
Switch Drive PWM Regulator Module*. The common 34 × 17 mm green module.

| Property | Value |
|---|---|
| Load supply | DC 5 – 36 V (we use 24 V) |
| Current | 15 A continuous, 30 A peak, 400 W; heatsink needed above ~10 A |
| Switch type | Two N-channel MOSFETs in parallel, **low-side**: OUT+ is internally tied to DC+, the MOSFETs switch OUT− |
| Trigger / PWM input | **Digital 3.3 – 20 V** ⇒ drives directly from an ESP32 GPIO |
| PWM frequency | 0 – 20 kHz |
| Isolation | None. The trigger GND is the same node as DC− |
| Terminals | Screw terminals DC+, DC− (supply), OUT+, OUT− (load); 2-pin header PWM, GND |
| Size | 34 × 17 × 12 mm |

One module per wheel. Details in `wiring/04-launch-wheels-dc-motors.md`.

> ❓ **OPEN:** Before I finalize the wheel wiring I need two quick checks on one module:
> continuity between DC+ and OUT+ (confirms low-side switching), and the PWM frequency the Uno
> sketch used (`analogWrite` default is 490 Hz, 980 Hz on pins 5/6, unless changed).

## 10. Electrolytic capacitor 1000 µF / 50 V (×2)

Described as sitting **between the MOSFET module output and the motor**, together with the
TVS (#11). Concern: after a PWM switch, an electrolytic capacitor is charged and discharged
on every PWM cycle. That means a large ripple current that heats the capacitor and shortens
its life, plus a current spike through the MOSFETs each time they turn on. The correct place
for a 1000 µF buffer is across the module's **input** (DC+/DC−), where it smooths what the
buck and the series diode have to deliver.

> ❓ **OPEN:** Before I finalize the wheel-branch diagram I need to know exactly where each
> 1000 µF capacitor is connected today (across OUT+/OUT−, or across DC+/DC−). If it is on the
> output side, `wiring/04-launch-wheels-dc-motors.md` proposes moving it.

## 11. TVS diode 1.5KE30A (×2)

Unidirectional transient-voltage-suppressor, 1500 W peak pulse power. Stand-off 25.6 V,
breakdown 28.5–31.5 V, clamping 41.4 V at 36 A. Connected across each 775 motor.

With a 24 V rail the TVS is reverse-biased in normal operation (small margin: 24 V against a
25.6 V stand-off). When the MOSFET switches off, the motor's inductance drives OUT− above
OUT+ and the TVS conducts in its forward direction: it is acting as the **flyback diode**
for the motor. That works, but a TVS is not designed for a continuous freewheeling current at
PWM rates and will run warm at higher wheel power. `wiring/04-launch-wheels-dc-motors.md`
suggests adding a real flyback Schottky per motor (a spare E83-004 is ideal) and keeping the
TVS for over-voltage protection.

> ❓ **OPEN:** Before I draw it I need the TVS orientation confirmed: the cathode band must be
> on the OUT+ (positive) side of the motor.

## 12. MEAN WELL N7805-1CW switching regulator

MEAN WELL N78 series, a non-isolated DC-DC switcher in the pin-out of an LM7805
(SIP-3: Vin – GND – Vout). Input 8–36 V, output **5 V / 1 A** (5 W), efficiency up to 96 %,
no heatsink, protections: short circuit, overload, over-temperature. "2452" on the part is
the date code (2024, week 52); "-1CW" is the package/revision suffix, same ratings as -1C.

Used in the prototype: 24 V (PSU output 1) → 5 V → both servos.

Concern: 1 A total for two 15 kg servos is marginal. When a servo stalls, the regulator's
overload protection pulls the 5 V rail down, the servos twitch and lose position. The
prototype got away with it because the feeder loads are light. Two acceptable layouts:

- **Recommended:** dedicate this N7805 to the **ESP32** (it needs ≤ 0.6 A) and add a
  5–6 V, ≥ 3 A buck converter for the servos (6 V gives the servos more torque and speed).
- Alternative: keep it for the servos only, add a 1000 µF capacitor on its output, and power
  the ESP32 from a second N7805-1C. Never share one N7805 between servos and ESP32.

> ❓ **OPEN:** Confirm the marking reads N7805 (5 V), see #4.

## 13. Optical endstop (from the Sidewinder X3 Plus)

An Artillery-style optical endstop board: a slotted infrared photo-interrupter, an indicator
LED and a 3-wire connector (VCC, GND, SIGNAL). Unlike a mechanical micro switch it needs
power and produces an active logic level rather than a dry contact; the level flips when a
flag enters the slot. Which level means "blocked" depends on the board and is established by
a two-minute test. Details in `wiring/06-optical-endstop.md`.

Level-shifting matters here: if the board is powered from 5 V its SIGNAL swings to 5 V, which
must not reach an ESP32 pin. Plan A is to power it from 3V3 (most of these boards work),
plan B is a resistor divider on SIGNAL.

> ❓ **OPEN:** Before I finalize the endstop page and the pin map I need a photo of the board
> (or its pin labels and any part numbers), whether it lights/switches when powered from 3.3 V,
> and what Tilen intends it for: pan homing, tilt homing, or detecting a shuttle in the feeder.
> Only one is available, so the other axis needs a second switch.

## 14. Arduino Uno (prototype controller)

Kept only as a reference. Everything the Uno sketches encode (step rates, accelerations,
servo angles, feed timing, PWM duty for a given launch distance) is directly reusable in
the ESP32 firmware. Note the Uno is 5 V logic; none of its 5 V wiring may be reused on
ESP32 inputs.

> ❓ **OPEN:** Before I write the firmware I need the Uno sketches from the prototype. Please
> copy them into `controller/reference-uno/` (any state, they do not need to be clean).

---

## Parts probably still needed (suggestions, not yet confirmed)

| Part | Why |
|---|---|
| 5–6 V buck converter, ≥ 3 A, fed from the 24 V rail | Servo supply; the N7805 (1 A) moves to the ESP32. See #12 |
| Second endstop (optical or mechanical) | #13 covers one axis; pan and tilt both need homing |
| Flyback Schottky per wheel motor (a spare E83-004 works) | Relieves the TVS of the freewheeling current. See #11 |
| Physical E-stop or switch in the 24 V wheel rail | Cuts the wheels without killing the ESP32 |
| Fuses: 10 A per wheel branch, optional 4 A for the stepper branch | Wiring protection on the 24 V rails |
| 470–1000 µF capacitor on the servo rail | Absorbs servo start/stall spikes |
| 0.1 µF ceramic capacitor across each 775 motor | Brush noise that otherwise disturbs Wi-Fi and step signals |
| Wire ≥ 1.5 mm² for the wheel branches | Up to 10 A per branch at 24 V, more at spin-up |
| Optional: shuttle-present sensor, RPM sensor per wheel | Feed confirmation, repeatable launch speed |

## Open questions checklist

Short version of every `❓ OPEN:` above, to answer in one go:

1. ESP32 board: photo or pin labels; one or two USB-C ports; RGB LED pin; USB-serial chip; antenna type.
2. DRV8825: sense-resistor marking, heatsinks, current Vref and microstep settings per driver.
3. Steppers: label text of each motor, which motor is on which axis, gear/belt ratios, what the feeder stepper does.
4. Servos: confirm the regulator is N78**05** (5 V) and measure the servo supply; feed sequence with angles and delays.
5. 775 wheels: highest PWM value used on the Uno; measured buck output voltage; independent wheel speeds or not.
6. PSU: model label; continuity between the two outputs' negatives; mains switch; fan.
7. Buck converter: current variant bought; output voltage under load; current-limit or shutdown behaviour.
8. E83-004: one or both elements used; anode toward the buck.
9. MOSFET module: DC+ ↔ OUT+ continuity; PWM frequency used on the Uno.
10. 1000 µF capacitors: across the module output or the module input?
11. TVS 1.5KE30A: cathode band toward OUT+?
12. Optical endstop: photo/pin labels; works at 3.3 V?; intended role.
13. Uno prototype sketches → `controller/reference-uno/`.
