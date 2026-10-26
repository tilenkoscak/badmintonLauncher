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
| 1 | ESP32-S3 dev board (DevKitC-1 clone) with ESP32-S3-WROOM-1-N16R8, 44 pins, two USB-C | 1 | Main controller: Wi-Fi access point, web server, all motion | Verified from a photo (2026-10-20): DevKitC-1 v1.1 header, PCB antenna |
| 2 | DRV8825 stepper driver carrier, 100 µF/50 V capacitor already fitted | 3 | Pan, tilt and feeder steppers | Vref 0.6 V on all three, works on the prototype. Sense-resistor marking not read |
| 3 | NEMA 17 stepper from the Sidewinder X3 Plus, label "42SHDC1067Z-15WD" | 3 | Pan (≈ 1:36.57 planetary), tilt (1:72), feeder carriage | Rated current not published; runs at Vref 0.6 V |
| 4 | Miuzei 15 kg digital servo, metal gear, 180° | 2 | Feeder: arms servo (gripper) and lift servo (rack and pinion), both on the carriage | 4.8-7.4 V, powered at 5 V. Angles: arms 145°/154°, lift 160°/0° |
| 5 | 775 brushed DC motor, 12 V, 150 W, 10 000 rpm | 2 | Launch wheels, both always at the same speed | Known from listing. Moving to a 12 V rail, see #7 |
| 6 | Artillery X3 Plus power supply, two outputs: 24 V / 4.2 A and 36 V / 9.7 A | 1 | Everything | Ratings known; model label and output isolation to verify |
| 7 | Buck converter for the wheel rail: 36/48 V → 12 V, 30 A, IP68 potted module | 1 | Wheel rail | **Ordered.** Replaces the 36 → 24 V module, which stays in until it arrives |
| 8 | Fuji Electric E83-004 Schottky diode, 40 V, 60 A, TO-3P | 2 | Flyback diode across each wheel motor (MOSFET module output) | Known, correctly placed |
| 9 | Dual-MOSFET PWM switch module, DC 5–36 V, 15 A (30 A peak), 400 W | 2 | Speed control of each launch wheel | Known; trigger 3.3–20 V, PWM 0–20 kHz |
| 10 | Electrolytic capacitor 1000 µF / 50 V | 2 | Buffer across each MOSFET module input | Known, correctly placed |
| 11 | TVS diode 1.5KE30A | 2 | Over-voltage clamp across each MOSFET module input | Known, correctly placed |
| 12 | MEAN WELL N7805-1CW switching regulator, 5 V / 1 A | 1 | 24 V → 5 V, today for the servos | Confirmed 5 V. **Undersized for two 15 kg servos** |
| 13 | Endstop sensor from the Sidewinder X3 Plus, called "optical endstop" so far; most likely its inductive bed-levelling sensor | 1 | Home of the feeder carriage and overtravel guard | Logic known from the Uno code (LOW when triggered); position known (front of the travel); type and supply voltage to confirm with a steel test |
| 14 | Arduino Uno | 1 | Prototype controller, being replaced | Its code is in `controller/reference-uno/` |

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
| Power in | USB-C 5 V, or the `5Vin` header pin, or regulated 3.3 V into `3V3` (not recommended) |

Role in this project: runs as a Wi-Fi access point, serves the web app to the phone,
receives commands over WebSocket, generates STEP/DIR for three DRV8825s, PWM for two servos
and PWM for two launch-wheel MOSFET modules. Details in `wiring/01-esp32-s3-basics.md`.

Checked on Tilen's board (photo of the top side, 2026-10-20):

| Check | Result |
|---|---|
| Module | **ESP32-S3-WROOM-1, N16R8** printed on the can. PCB antenna (WROOM-1, not the u.FL WROOM-1U); the antenna end overhangs the board edge |
| Header | 2 × 22 pins, silkscreen identical to the DevKitC-1 v1.1 layout drawn in `wiring/01-esp32-s3-basics.md`. The 5 V pin is labelled `5Vin`, the reset button `RET` |
| USB | Two USB-C ports at the end opposite the antenna, not labelled on the top side. The USB-serial chip sits next to the port on the J3 side, so that one is most likely COM and the other native USB. How to tell them apart for sure: `wiring/01-esp32-s3-basics.md` |
| USB-serial chip | Small QFN package, marking not legible in the photo. These clones normally carry a CH343P |
| RGB LED | Fitted, beside J1 near GPIO 10/11. Whether it is wired to GPIO 48 or 38 cannot be seen from the top; smoke test 1 settles it |

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

Measured by Tilen (2026-10-20): **Vref = 0.6 V on all three drivers**, the setting the
prototype runs with. The sense-resistor marking was not read. With `R100` (0.1 Ω), which
almost every DRV8825 carrier uses, 0.6 V is a limit of **1.2 A per phase**; with `R200` it
would be 0.6 A. The motors work at this setting, so it stays. The microstep pins get jumpers
on the ESP32 build (1/8 on all three, `wiring/pin-map.md`), so how the Uno wired them no
longer matters.

## 3. Stepper motors (×3: pan, tilt, feeder)

All three are salvaged from the **Artillery Sidewinder X3 Plus**. Two are the Z-axis motors
(the printer has dual Z), the third comes from another axis of the same printer and is
believed to be the same size and rating.

| Property | Value |
|---|---|
| Frame | NEMA 17 (42 mm). Artillery lists the X3/X4 Z-axis motor as type **"42-35"** (42 mm square, 35 mm long) |
| Label | **42SHDC1067Z-15WD**, LOT NO. 231009R, Artillery logo. Read from one motor (photo, 2026-10-20); the other two are assumed to be the same type |
| Step angle | 1.8° (200 full steps per revolution) assumed, standard for these printers; the label does not state it |
| Rated current | **Not published.** No datasheet was found for 42SHDC1067Z-15WD. The working value is the prototype's Vref 0.6 V (≈ 1.2 A with R100 sense resistors, see #2) |
| Wiring | Bipolar, 4 wires, Artillery motors come with a JST-XH plug (4-pin, or 6-pin with 4 populated) |

Gearing between motor and axis (Tilen, 2026-10-20), with the step counts at the 1/8
microstepping of the ESP32 build (1600 microsteps per motor revolution):

| Axis | Gearing | Microsteps per output degree |
|---|---|---|
| Pan | 2-stage planetary, **≈ 1:36.57** | 1600 × 36.57 / 360 ≈ **162.5** |
| Tilt | **1:72** | 1600 × 72 / 360 = **320** |
| Feeder carriage | positions are taught in steps, no ratio needed | 1600 per motor revolution |

The pan ratio is approximate, so the degrees the app shows for pan are approximate too.
Positions and limits are stored in microsteps, so this does not affect repeatability.

What the Uno code adds (`controller/reference-uno/README.md`): the feeder driver ran at
1/8 microstepping with M0 and M1 tied together and driven HIGH, and the old feeder homed
against the optical endstop (#13). In the redesigned feeder the stepper moves a carriage
between PARK, the shuttle tube and the wheels (`controller/README.md`).
Pan and tilt ran at a gentle 50 RPM motor speed, which through the gearing is only about
8°/s of pan and 4°/s of tilt.

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

Powered at **5 V** from the 24 V rail through the MEAN WELL N7805 (#12). Confirmed correct;
the "12 V" in the first prototype notes was a slip.

Roles in the **redesigned feeder** (2026-10). Both servos ride on the carriage that the
feeder stepper moves between PARK, the shuttle tube and the wheels:

| Servo | GPIO | Mechanism | Positions (Tilen, 2026-10-20) |
|---|---|---|---|
| Arms servo | 10 | Two arms coupled by gears; closing them grabs the shuttle at the cork, at the bottom of the shuttle tube | OPEN **145°**, CLOSED **154°** |
| Lift servo (formerly "spoon") | 11 | A pinion on the servo drives a vertical gear rack; the rack carries the arms servo | BOTTOM **160°**, TOP **0°** |

The feed cycle and the park routine are specified in `controller/README.md` ("Feeder state
machine"). The Uno-era angles (arms 10°/40°, spoon 180°/105°/0°) belong to the old mechanism.

Tilen's understanding is that these servos use **0-160°**, so the firmware clamps every servo
command to that range (`SERVO_MAX_DEG = 160`). The angles were found on the Uno, in the scale
of its `Servo` library (0-180° = 544-2400 µs), and the firmware uses the same pulse range so
they carry over unchanged (`wiring/03-servos.md`). Settle times were not measured: the firmware starts with
300 ms for the arms and 800 ms for the lift, both adjustable in the app.

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
MOSFET modules (#9). The module switches one direction only, so the counter-rotation comes
from swapping the two motor leads on one wheel. Details in
`wiring/04-launch-wheels-dc-motors.md`.

**Rail voltage:** in the prototype these 12 V motors hang on the **24 V** output of the old
buck converter, where 100 % PWM duty would run them at twice their rating. A 12 V / 30 A buck
is ordered (#7). The firmware derives its duty cap from `WHEEL_RAIL_V` and ships with 24
(cap 50 %) until the new buck is fitted and measured.

The only wheel code on the Uno is a single-motor test (`launcher_motors_control.ino`): PWM
on pin 5 at 980 Hz, a 200 ms kick at duty 255, then duty 100 (39 %) and a ramp to 255 and
back. So a 775 has already run at full duty on the 24 V rail, but there is no record of which
duty produced a good shot.

Tilen (2026-10-20): the duty that gives a good shot is **not known yet**. It will be found by
testing with the web app's manual wheel control, on the 12 V rail once it is fitted. If full
duty on 12 V turns out too slow, the rail question reopens. **Both wheels always run at the
same speed**, so the app has one speed setting. They keep separate PWM lines so the firmware
can start the second wheel a moment after the first.

## 6. Power supply: Artillery Sidewinder X3 Plus PSU (dual output)

Salvaged from the printer. Mains input, two DC outputs:

| Output | Rating | Power | Used for (prototype) |
|---|---|---|---|
| 1 | **24 V / 4.2 A** | ≈ 100 W | Three DRV8825 (VMOT) and, via the N7805, the two servos |
| 2 | **36 V / 9.7 A** | ≈ 350 W | Buck converter (#7) → launch wheels |

Because the supply is a mains unit, the robot is treated as **mains-powered**; the battery
option from the first draft is dropped.

Measured by Tilen (2026-10-21): **no continuity between the - of output 1 and the - of
output 2**, so the two outputs are isolated from each other inside the PSU. Consequence: the
star point is the **only** connection between the 24 V side (steppers, N7805, ESP32, servos)
and the wheel side (buck, MOSFET modules). Without that wire the ESP32's PWM signal has no
reference at the MOSFET modules. See `wiring/05-power.md`.

> ❓ **OPEN:** Before I finish `wiring/05-power.md` I need a photo of the PSU label (model
> number and ratings), so I can look up its datasheet, and whether the PSU has a mains switch
> and came with a fan.

## 7. Buck converter for the wheel rail: 12 V / 30 A ordered

The module in the prototype, AliExpress 1005006451305852: *DC DC 36V 48V to 24V 1A 2A 3A 5A 10A 15A 20A 30A Step Down
… 480W Buck Converter IP68 Waterproof*. A potted aluminium module with wire leads,
**fixed 24 V output**, sold in current variants from 1 A to 30 A; the 36/48 V version accepts
30–60 V input. Non-isolated (input − and output − are the same node) as is usual for this type.

Fed from PSU output 2 (36 V / 9.7 A ≈ 350 W), so the rail can deliver at most about
**13 A at 24 V** or **26 A at 12 V** (≈ 90 % efficiency) before the PSU limits.

**Decision (2026-10-05): a 12 V / 30 A module of the same IP68 family is ordered**
(sold as "36V/48V to 12V 30A 360W"). It replaces the 24 V module, which stays in until the
new one arrives and then becomes a spare. Reasons:

- With a 12 V rail the maximum wheel speed is limited by hardware. On 24 V it is limited
  only by a firmware constant; one bug or one test sketch without the cap spins the disks at
  ~20 000 rpm, where rim stress is four times the design value.
- The motors run inside their rating: brushes and commutator see 12 V pulses instead of 24 V
  ones, less arcing, less heat, less electrical noise.
- The full duty range becomes usable; the firmware's cap becomes 100 %.
- 30 A rather than 20 A because the PSU can deliver ≈ 26 A at 12 V; a 20 A module would
  become the bottleneck during spin-up.
- Everything else in the wheel chain (#8–#11) is fine at 12 V; nothing else changes.
  The 24 V module becomes a spare.

When it arrives: measure its output without load and with one wheel at 50 % duty (expect
12 V ± 0.5 V), then set `WHEEL_RAIL_V = 12` in the firmware. The full commissioning list is
in `wiring/04-launch-wheels-dc-motors.md`, "Until the new buck arrives".

## 8. Schottky diode Fuji Electric E83-004 (×2), flyback across each motor

AliExpress 1005009025204329: *E83-004 TO-3P 60A 40V Schottky Rectifier Diode*. Fuji
Electric Schottky barrier diode: 40 V reverse, 60 A total as two 30 A elements in a
3-terminal TO-3P package (anode – common cathode – anode), forward drop about 0.55 V.

Position: **across the OUT+ / OUT− terminals of each MOSFET module**, i.e. across the motor,
cathode to OUT+ and anode to OUT− (any other orientation would have shorted the rail).
Function: **flyback diode**. When the MOSFET switches off, the motor's inductance keeps the
current flowing and this diode gives it a path, instead of letting the voltage spike.
It carries the motor current during the PWM off-time only, so it dissipates roughly
`0.55 V × I_motor × (1 − duty)`: about 1–2 W at full wheel power. A small heatsink or none.
Hugely over-rated at 60 A, which is fine. Correctly placed and sized.

## 9. Dual-MOSFET PWM switch module, 15 A / 400 W (×2)

AliExpress 1005007594739156: *DC 5V-36V 15A Max 30A 400W Dual High-Power MOSFET Trigger
Switch Drive PWM Regulator Module*. The common 34 × 17 mm green module.

| Property | Value |
|---|---|
| Load supply | DC 5 – 36 V (12 V once the new buck is fitted, 24 V until then) |
| Current | 15 A continuous, 30 A peak, 400 W; heatsink needed above ~10 A |
| Switch type | Two N-channel MOSFETs in parallel, **low-side**: OUT+ is internally tied to DC+, the MOSFETs switch OUT−. Confirmed by measurement (DC+ to OUT+ beeps, 2026-10-21) |
| Trigger / PWM input | **Digital 3.3 – 20 V** ⇒ drives directly from an ESP32 GPIO |
| PWM frequency | 0 – 20 kHz |
| Isolation | None. The trigger GND is the same node as DC− |
| Terminals | Screw terminals DC+, DC− (supply), OUT+, OUT− (load); 2-pin header PWM, GND |
| Size | 34 × 17 × 12 mm |

One module per wheel. Details in `wiring/04-launch-wheels-dc-motors.md`.

The Uno test drove the module at 980 Hz (`analogWrite` on pin 5), so the module is proven at
that frequency; the ESP32 will use ≈ 16 kHz, inside the module's 0–20 kHz rating.

Checked by Tilen (2026-10-21): DC+ and OUT+ are connected (the multimeter beeps), so the
module switches the low side as assumed. OUT- and the motor terminals are therefore never
ground and must never be tied to it.

## 10. Electrolytic capacitor 1000 µF / 50 V (×2)

**Across DC+ / DC− of each MOSFET module** (the input side). Correct place: it buffers the
PWM current pulses so the buck and the wiring see a smooth current, and it does not sit
behind the switch where it would be charged and discharged every cycle. 50 V rating is
fine for 24 V and for 12 V.

## 11. TVS diode 1.5KE30A (×2)

Unidirectional transient-voltage-suppressor, 1500 W peak pulse power. Stand-off 25.6 V,
breakdown 28.5–31.5 V, clamping 41.4 V at 36 A. **Across DC+ / DC− of each MOSFET module**,
cathode (band) to DC+. Function: clamps voltage transients on the wheel rail (switching
spikes, buck overshoot) before they reach the module, which is rated for 36 V.

On the 24 V rail the margin to the 25.6 V stand-off is small but adequate; on a 12 V rail it
is comfortable. Correctly placed.

## 12. MEAN WELL N7805-1CW switching regulator

MEAN WELL N78 series, a non-isolated DC-DC switcher in the pin-out of an LM7805
(SIP-3: Vin – GND – Vout). Input 8–36 V, output **5 V / 1 A** (5 W), efficiency up to 96 %,
no heatsink, protections: short circuit, overload, over-temperature. "2452" on the part is
the date code (2024, week 52); "-1CW" is the package/revision suffix, same ratings as -1C.
Confirmed to be the 5 V part.

Used in the prototype: 24 V (PSU output 1) → 5 V → both servos.

Concern: 1 A total for two 15 kg servos is marginal. When a servo stalls, the regulator's
overload protection pulls the 5 V rail down, the servos twitch and lose position. The
prototype got away with it because the feeder loads are light. Two acceptable layouts:

- **Recommended:** dedicate this N7805 to the **ESP32** (it needs ≤ 0.6 A) and add a
  5–6 V, ≥ 3 A buck converter for the servos (6 V gives the servos more torque and speed).
- Alternative: keep it for the servos only, add a 1000 µF capacitor on its output, and power
  the ESP32 from a second N7805-1C. Never share one N7805 between servos and ESP32.

## 13. Optical endstop (from the Sidewinder X3 Plus)

Assumed so far to be a slotted optical endstop. **The photo (2026-10-23) shows otherwise:** a
small black sensor body with a round sensing face at one end, a mounting hole, an indicator
LED and a three-wire cable with a white 3-pin plug. That matches the X3 Plus's **inductive
bed-levelling sensor** (Artillery: "Inductive Levelling Sensor for SW-X3/SW-X4"), a proximity
sensor that reacts to metal, steel best, a few millimetres in front of its round face.
Artillery publishes no voltage, output type or pinout for it. Like any such sensor it needs
power and produces an active logic level rather than a dry contact.

Role, from the Uno code: it is the **feeder's home sensor and safety limit** ("feeding
limiter", Uno pin 4). The feeder stepper homes against it at start-up, and an unexpected
trigger during operation stops everything. It was read as a plain input without pull-up and
is **LOW when triggered**. Pan and tilt had no endstops; their limits were taught with
buttons. Details in `wiring/06-optical-endstop.md`.

Position (Tilen, 2026-10-20): at the **front** of the carriage travel, towards the wheels. It
marks the furthest the feeding mechanism may move: the shuttle still reaches the flywheels
and the mechanism does not jam into them. The carriage therefore homes towards the wheels.

Level-shifting matters here: on the Uno the sensor ran from 5 V, so its SIGNAL swung to 5 V,
which must not reach an ESP32 pin. Plan A is to power it from 3V3 (inductive sensors often
need more), plan B is 5 V with a resistor divider on SIGNAL.

> ❓ **OPEN:** Before I finalize the endstop page I need the bench test in
> `wiring/06-optical-endstop.md` done with a **steel object** against the round face (the
> earlier attempts used a card, which this sensor ignores): SIGNAL voltage at rest and with
> steel, on 3.3 V and on 5 V. Also the three wire colours, and which part of the prototype's
> carriage triggered it.

## 14. Arduino Uno (prototype controller)

Kept only as a reference. Its code (a PlatformIO project and a set of Arduino IDE sketches)
is in `controller/reference-uno/`, summarised in the README there: servo angles, feed
sequence, homing routine, stepper speeds and the wheel test are all extracted. Note the Uno is
5 V logic; none of its 5 V wiring may be reused on ESP32 inputs. This particular Uno's
pins 10 and 11 were suspected faulty as inputs, which is irrelevant for the ESP32 build.

---

## Parts probably still needed (suggestions, not yet confirmed)

| Part | Why |
|---|---|
| 5–6 V buck converter, ≥ 3 A, fed from the 24 V rail | Servo supply; the N7805 (1 A) moves to the ESP32. See #12 |
| A clear START mark per axis on the frame (pointer + line) | After every power-up pan and tilt are jogged to these marks and confirmed in the app (`controller/README.md`). Home sensors are not planned |
| Physical E-stop or switch in the wheel rail | Cuts the wheels without killing the ESP32 |
| Fuses: 15 A per wheel branch (12 V rail), optional 4 A for the stepper branch | Wiring protection |
| 470–1000 µF capacitor on the servo rail | Absorbs servo start/stall spikes |
| 0.1 µF ceramic capacitor across each 775 motor | Brush noise that otherwise disturbs Wi-Fi and step signals |
| Wire 2.5 mm² for the wheel branches | Spin-up peaks of tens of amps on the 12 V rail |
| Flexible servo extension leads, slack loop or small drag chain | Both servos ride on the moving carriage, the arms servo also moves up and down |
| Optional: shuttle-present sensor, RPM sensor per wheel | Feed confirmation, repeatable launch speed |

## Open questions checklist

Short version of every `❓ OPEN:` still in the repo, to answer in one go:

1. Endstop sensor (#13): bench test with a steel object against the round face, on 3.3 V and on 5 V; wire colours; what on the prototype's carriage triggered it (`wiring/06-optical-endstop.md`).
2. PSU: photo of the label; mains switch; fan (#6).
