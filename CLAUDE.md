# Badminton Shuttle Launcher

A badminton shuttle launcher controlled from a phone. A working mechanical/electrical
prototype already exists, built around an **Arduino Uno** and parts salvaged from an
**Artillery Sidewinder X3 Plus** 3D printer (steppers, dual-output PSU, optical endstop).
This repo is about replacing the Uno with an **ESP32-S3** and building a **phone web app**
that controls the whole robot over Wi-Fi.

Owner: Tilen. Language of code, comments and docs: English.

## What the machine does

| Subsystem | Actuator | Driver / supply |
|---|---|---|
| Pan (rotate left/right) | NEMA 17 stepper (Sidewinder Z-axis motor) | DRV8825 on 24 V |
| Tilt (up/down) | NEMA 17 stepper (Sidewinder Z-axis motor) | DRV8825 on 24 V |
| Feeder (takes one shuttle from the tube and pushes it into the wheels) | A carriage on a NEMA 17 stepper, homed on the optical endstop. On the carriage: the lift servo (rack and pinion, vertical) carrying the arms servo (geared gripper that grabs the shuttle at the cork). Both servos are Miuzei 15 kg | DRV8825 on 24 V; ESP32 PWM, 5–6 V servo rail |
| Launch wheels (two counter-rotating disks) | 2 × 775 brushed DC motors, 12 V, 150 W, 10 000 rpm | 36 V → 12 V / 30 A buck (ordered; the old 24 V buck until it arrives), 15 A low-side MOSFET PWM module per wheel with 1000 µF + TVS on its input, E83-004 Schottky flyback across each motor |

Every subsystem works **individually** on the Uno prototype. Nothing is integrated yet;
that integration (one controller + one UI) is the goal of this repo.

## Repo layout

| Path | Purpose |
|---|---|
| `CLAUDE.md` | This file: project summary + working conventions |
| `PARTS.md` | Bill of materials: every electronic part, what is known, what is still unknown |
| `wiring/` | How to wire each subsystem to the ESP32-S3. Tilen has never used an ESP32 before, so these docs explain the differences from the Uno, not just the connections. `wiring/wiring-diagrams.html` shows every diagram on one page in a browser |
| `controller/` | The controller itself: `firmware/` (ESP32-S3 PlatformIO project), `webapp/` (the phone UI the ESP32 serves) and `reference-uno/` (the Uno prototype code; its README extracts speeds, homing and the old feeder's angles) |

## Key technical constraints

- The ESP32-S3 is **3.3 V logic and NOT 5 V tolerant**. The Uno was 5 V. Anything that feeds a
  signal *into* the ESP32 must be at or below 3.3 V. Outputs at 3.3 V are fine for DRV8825
  (logic high threshold 2.2 V), for the servos and for the MOSFET modules (trigger 3.3–20 V).
- Board variant is **N16R8** (16 MB flash, 8 MB *octal* PSRAM). On this variant
  **GPIO 35, 36 and 37 are used by the PSRAM** and must never be used, even though they are
  on the header. Also avoid strapping pins 0, 3, 45, 46, the USB pins 19/20 and UART0 pins 43/44.
- Power comes from the Artillery PSU: **24 V / 4.2 A** (steppers, ESP32 via a MEAN WELL N7805,
  servo buck) and **36 V / 9.7 A** → buck → wheel rail. The robot is mains-powered.
- **The wheel rail becomes 12 V**, matching the 775 motors: a 12 V / 30 A buck is ordered to
  replace the 24 V one. Firmware derives its duty cap from a constant:
  `WHEEL_MAX_DUTY = min(100 %, 12 V / WHEEL_RAIL_V)`. It ships with `WHEEL_RAIL_V = 24`
  (cap 50 %) and is set to 12 only after the new buck is fitted and measured. The cap is a
  safety constant, not a setting. Wheel PWM runs near the module limit (`WHEEL_PWM_HZ` ≈ 16 kHz).
- The N7805 delivers 1 A. The ESP32 gets it alone; the servos get their own ≥ 3 A buck.
  The ESP32 must never share a regulator with the servos (brown-out resets).
- **Feeder motion order is safety-relevant:** the carriage only moves with the lift at
  BOTTOM, and the arms only open once the carriage is away from the wheels. The cycle and
  the park routine are specified in `controller/README.md` ("Feeder state machine"); do not
  reorder steps without checking it.
- Phone ↔ robot link is **Wi-Fi**: the ESP32 runs as an access point and serves the web app.
  Target: an **Android phone with the Brave browser** (Chromium); iOS is not a target.
  Bluetooth is not used.
  Consequence: the phone has **no internet** while connected, so the web app must be fully
  self-contained (no CDN scripts, fonts or icons).
- The launch wheels spin at up to 10 000 rpm and the 775 motors can draw tens of amps.
  Every control path must **fail safe**: lost connection or crashed UI ⇒ wheels off and
  feeder parked.
- Motor power never flows through the ESP32 board. Logic and motor supplies share only a
  common ground at one star point.

## Working conventions

- Documentation grows incrementally. Facts we do not have yet are marked inline as a
  blockquote starting with `❓ **OPEN:**`, written as an instruction to Claude
  ("Before I can … I need to know …"). Search the repo for `OPEN:` to list all of them.
  When Tilen answers, replace the note with the confirmed fact. Never keep both.
- Do not invent component ratings. If a current, voltage or pin is not confirmed, say so.
- Firmware: Arduino framework (arduino-esp32 core 3.x) on the ESP32-S3, built with
  **PlatformIO** in VS Code or Cursor, like Tilen's Uno project. Board configuration is in
  `wiring/01-esp32-s3-basics.md`.
- `wiring/pin-map.md`, `PARTS.md`, `wiring/wiring-diagrams.html` and the firmware pin
  definitions must stay in sync. A pin change is done in all four places in the same commit.
- Prefer small, testable steps: each wiring doc ends with a smoke test that proves that
  one subsystem works on the ESP32 before the next one is connected.
- Tilen also works from a second computer: fetch before editing, the remote may be ahead.

## Status (2026-10-20)

- [x] Prototype mechanics and all subsystems working individually on Arduino Uno
- [x] Repo skeleton, parts list, first draft of wiring docs and pin map
- [x] Component specs collected: PSU, buck, wheel power chain, servos, N7805, optical endstop
- [x] Uno prototype code collected in `controller/reference-uno/` and summarised
- [x] Wheel rail: 12 V / 30 A buck ordered
- [x] Toolchain: PlatformIO
- [x] Redesigned feeder documented: carriage, lift servo, arms servo, cycle and park routine
- [x] Board verified from a photo; Vref, motor label, gear ratios and servo angles recorded
- [x] Pin map v0.5 final; all wiring diagrams on one page (`wiring/wiring-diagrams.html`)
- [x] Web app v1 scope: Android + Brave, complete manual control, pan/tilt START/END, no drills
- [ ] Resolve the remaining `OPEN:` questions (endstop sensor: steel test on 3.3 V and 5 V; PSU label)
- [ ] ESP32 firmware skeleton: Wi-Fi AP, web server, WebSocket, motor drivers
- [ ] Web app v1: manual control of every axis and every feeder step, wheel speed, feed one, e-stop, pan/tilt START/END, feeder setup
- [ ] After v1: timed auto-feed, drills (random/programmed pan & tilt patterns)
