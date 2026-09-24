# Badminton Shuttle Launcher

A badminton shuttle launcher controlled from a phone. A working mechanical/electrical
prototype already exists, built around an **Arduino Uno** and a bench power supply.
This repo is about replacing the Uno with an **ESP32-S3** and building a **phone web app**
that controls the whole robot over Wi-Fi.

Owner: Tilen. Language of code, comments and docs: English.

## What the machine does

| Subsystem | Actuator | Driver |
|---|---|---|
| Pan (rotate left/right) | 1 stepper motor | DRV8825 |
| Tilt (up/down) | 1 stepper motor | DRV8825 |
| Feeder (drops one shuttle into the wheels) | 2 hobby servos + 1 stepper | ESP32 PWM + DRV8825 |
| Launch wheels (two counter-rotating disks) | 2 × 775 brushed DC motors, 12 V, 150 W, 10 000 rpm | PWM power stage (type still unknown) |

Every subsystem works **individually** on the Uno prototype. Nothing is integrated yet;
that integration (one controller + one UI) is the goal of this repo.

## Repo layout

| Path | Purpose |
|---|---|
| `AGENTS.md` | This file: project summary + working conventions |
| `PARTS.md` | Bill of materials: every electronic part, what is known, what is still unknown |
| `wiring/` | How to wire each subsystem to the ESP32-S3. Tilen has never used an ESP32 before, so these docs explain the differences from the Uno, not just the connections |
| `controller/` | The controller itself: `firmware/` (ESP32-S3 sketch), `webapp/` (the phone UI the ESP32 serves) and `reference-uno/` (prototype Uno sketches, to be ported) |

## Key technical constraints

- The ESP32-S3 is **3.3 V logic and NOT 5 V tolerant**. The Uno was 5 V. Anything that feeds a
  signal *into* the ESP32 must be at or below 3.3 V. Outputs at 3.3 V are fine for DRV8825
  (logic high threshold 2.2 V) and for nearly all hobby servos.
- Board variant is **N16R8** (16 MB flash, 8 MB *octal* PSRAM). On this variant
  **GPIO 35, 36 and 37 are used by the PSRAM** and must never be used, even though they are
  on the header. Also avoid strapping pins 0, 3, 45, 46, the USB pins 19/20 and UART0 pins 43/44.
- Phone ↔ robot link is **Wi-Fi**: the ESP32 runs as an access point and serves the web app.
  Bluetooth is not used (iOS browsers cannot reach it; Web Bluetooth is Chrome/Android only).
  Consequence: the phone has **no internet** while connected, so the web app must be fully
  self-contained (no CDN scripts, fonts or icons).
- The launch wheels spin at up to 10 000 rpm and the 775 motors can draw tens of amps.
  Every control path must **fail safe**: lost connection or crashed UI ⇒ wheels and feeder stop.
- Motor power never flows through the ESP32 board. Logic and motor supplies share only a
  common ground.

## Working conventions

- Documentation grows incrementally. Facts we do not have yet are marked inline as a
  blockquote starting with `❓ **OPEN:**`, written as an instruction to Codex
  ("Before I can … I need to know …"). Search the repo for `OPEN:` to list all of them.
  When Tilen answers, replace the note with the confirmed fact. Never keep both.
- Do not invent component ratings. If a current, voltage or pin is not confirmed, say so.
- Firmware uses the Arduino framework on ESP32-S3 (Tilen already knows Arduino).
  Arduino IDE vs PlatformIO is not decided yet, see `controller/README.md`.
- `wiring/pin-map.md`, `PARTS.md` and the firmware pin definitions must stay in sync.
  A pin change is done in all three places in the same commit.
- Prefer small, testable steps: each wiring doc ends with a smoke test that proves that
  one subsystem works on the ESP32 before the next one is connected.

## Status (2026-09-22)

- [x] Prototype mechanics and all subsystems working individually on Arduino Uno
- [x] Repo skeleton, parts list, first draft of wiring docs and pin map
- [ ] Resolve all `OPEN:` questions (component specs, DC motor driver, power supply)
- [ ] Final pin map and wiring diagrams
- [ ] ESP32 firmware skeleton: Wi-Fi AP, web server, WebSocket, motor drivers
- [ ] Web app v1: manual control of every axis, wheel speed, feed button, e-stop
- [ ] Drills: timed auto-feed, random/programmed pan & tilt patterns
