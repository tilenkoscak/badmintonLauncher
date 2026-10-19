# Controller: ESP32-S3 firmware + phone web app

Goal: one controller that runs the whole launcher, operated from a phone browser.
No code exists yet. This file records the planned architecture, the decisions taken and the
open ones, so the code can be built iteratively, one feature at a time.

## Layout

```
controller/
  README.md          this plan
  firmware/          PlatformIO project for the ESP32-S3           (to be created)
  webapp/            HTML/CSS/JS of the phone UI                    (to be created)
  reference-uno/     Tilen's Uno prototype code (PlatformIO project + IDE sketches), summarised in its README
```

## Architecture (planned)

```
   Phone browser                              ESP32-S3 (access point "ShuttleLauncher")
   ┌──────────────────────┐                   ┌─────────────────────────────────────────┐
   │  webapp (index.html) │  HTTP GET /       │  web server: serves the embedded UI     │
   │  - controls          │ ◄───────────────► │                                         │
   │  - status display    │  WebSocket /ws    │  command handler (JSON) ──► motion      │
   │  - heartbeat 2 Hz    │ ◄───────────────► │  status broadcast (JSON, 5–10 Hz)       │
   └──────────────────────┘                   │  safety watchdog: no heartbeat 1.5 s    │
                                              │      ⇒ wheels off, feeder parks         │
                                              │  motion: steppers (pan/tilt/feed),      │
                                              │          servos, wheel PWM              │
                                              └─────────────────────────────────────────┘
```

1. The ESP32 runs as a **Wi-Fi access point**. The phone joins it and opens
   `http://192.168.4.1`. No router, no internet, works on the court.
2. The ESP32 **serves the web app** (one self-contained HTML file embedded in the firmware).
3. The web app and the firmware talk over a **WebSocket**: JSON commands from the phone,
   JSON status back at 5–10 Hz (positions, wheel duty, feeder state, shot count, e-stop).
4. The firmware owns all motion and all safety logic. The phone is only a remote.

## Decisions taken

| Topic | Decision | Why |
|---|---|---|
| Link | Wi-Fi, ESP32 as access point. Optionally also join the home network during development | Works on iPhone and Android, no pairing, no app install. Bluetooth is not reachable from iOS browsers and Web Bluetooth is Chrome-only |
| Protocol | WebSocket carrying small JSON messages | Bidirectional, low latency, trivial in browser JS, easy to debug |
| UI delivery | Single `index.html` (CSS and JS inline), gzipped and embedded in the firmware as a byte array by a PlatformIO pre-build script | One upload, no filesystem image to flash, 16 MB flash is plenty |
| No external resources in the UI | Everything inline, no CDN, no web fonts | The phone has **no internet** while connected to the robot |
| Framework and toolchain | Arduino framework (arduino-esp32 core 3.x), **PlatformIO** in VS Code or Cursor (decided 2026-10-05) | Tilen knows Arduino, and his later Uno code is already a PlatformIO project. Library versions pinned in `platformio.ini`; one environment per smoke test, as in the Uno project |
| Libraries (candidates) | `ESPAsyncWebServer` + `AsyncTCP` (ESP32Async forks) for HTTP + WebSocket · `AccelStepper` first (the Uno code uses it, ports 1:1), `FastAccelStepper` if hardware-timed pulses are needed · `ESP32Servo` with the servo's own pulse range 500–2500 µs · built-in LEDC for wheel PWM · `Preferences` for settings in flash | Versions pinned when the firmware skeleton is started |
| Wheel rail and duty cap | 12 V rail (buck ordered). `WHEEL_MAX_DUTY = min(100 %, 12 V / WHEEL_RAIL_V)` with `WHEEL_RAIL_V = 24` until the new buck is fitted and measured. UI "100 %" maps to the cap | The 775 motors are 12 V (`wiring/04-launch-wheels-dc-motors.md`) |
| Wheel PWM frequency | `WHEEL_PWM_HZ` ≈ 16 kHz (10 kHz if the modules run hot), never above 20 kHz | Limit of the MOSFET modules; high frequency keeps current ripple low |
| Pan/tilt limits | Taught from the app (jog to each end, confirm) and stored in flash, as the prototype did with two buttons; endstops optional later | `reference-uno/README.md` |
| Feeder | Carriage homed against the optical endstop (`FEED_HOME_ACTIVE_LEVEL = LOW`). Carriage positions PARK, TUBE and WHEELS and the four servo angles are taught from the app and stored in flash | Redesigned mechanism, next section |
| Empty tube | No sensor in v1. The app counts shots against the number of shuttles the user says are loaded. An IR sensor on GPIO 40 (`SHUTTLE_SENSE`) can come later | With an empty tube the gripper simply closes on nothing, which the firmware cannot see |

## Feeder state machine

Mechanism (redesigned 2026-10; replaces the Uno-era "spoon" design): the feeder stepper
moves a **carriage** horizontally between the shuttle tube and the wheels. On the carriage,
the **lift servo** turns a pinion that drives a vertical gear rack. The rack carries the
**arms servo**, whose two geared arms close around the shuttle's cork. Each servo has two
fixed positions (arms OPEN/CLOSED, lift BOTTOM/TOP). The carriage has three taught
positions, counted in steps from the endstop:

| Carriage position | Meaning |
|---|---|
| `PARK` | Furthest back, clear of the shuttle tube. Rest position at start-up, at the end and after an e-stop |
| `TUBE` | Gripper directly under the shuttle tube |
| `WHEELS` | Shuttle between the flywheels |

**Start-up** (servo and carriage positions unknown, wheels off):

1. Arms → OPEN
2. Lift → BOTTOM
3. Home the carriage against the optical endstop, then carriage → PARK

**One shot** (repeated for auto-feed):

| # | Carriage | Lift | Arms | What happens |
|---|---|---|---|---|
| 1 | → TUBE | BOTTOM | | carriage moves under the tube |
| 2 | TUBE | BOTTOM | → OPEN | already open on the first shot |
| 3 | TUBE | → TOP | OPEN | arms rise around the cork of the lowest shuttle |
| 4 | TUBE | TOP | → CLOSED | arms grab the cork |
| 5 | TUBE | → BOTTOM | CLOSED | the shuttle is pulled out of the tube |
| 6 | → WHEELS | BOTTOM | CLOSED | **only when the wheels are at speed**; the wheels pull the shuttle out of the gripper and launch it |

Then back to step 1. This follows Tilen's description. Working assumption: the wheels'
friction pulls the shuttle out of the closed gripper. If testing shows it does not, the
fallback is one extra step, arms → OPEN at WHEELS (`FEED_RELEASE_AT_WHEELS`).

**Park** (STOP, end of session, lost connection, E-STOP), from wherever the feeder is:

1. If the lift is at TOP (that only happens at TUBE): arms → OPEN, so the shuttle stays in the
   tube, then lift → BOTTOM
2. Carriage → PARK at reduced speed, arms as they are
3. Arms → OPEN

Two rules follow from this and hold everywhere in the firmware:

- The carriage never moves while the lift is up; that would tear the gripper against the
  tube. The lift only goes to TOP at TUBE.
- The arms only open once the carriage is away from the wheels, so a shuttle still held at
  WHEELS is not dropped into spinning disks.

At start-up the firmware does not know the state, so it uses the simpler order above. That is
safe because the wheels are off at power-up.

Servos do not report their position, so each servo move waits a fixed settle time
(`SERVO_SETTLE_MS`, found during setup; the old mechanism needed 300–750 ms per move).

Homing direction depends on where the endstop sits. On the Uno-era feeder it was at the
front end of the stroke, towards the wheels. Homing therefore only runs with the wheels off,
wherever the endstop is.

> ❓ **OPEN:** Before I can write the homing routine I need to know at which end of the
> carriage travel the optical endstop sits now (behind PARK, or in front of WHEELS) and
> roughly how far PARK, TUBE and WHEELS are from it. Millimetres or stepper revolutions are
> both fine; the exact values get taught in the app.

## Decisions still open

> ❓ **OPEN:** Before I write the web app I need to know which phone(s) it must run on
> (Android, iPhone, both) and the browser. iOS Safari behaves differently on access points
> without internet (captive-portal popup), which affects how the app is opened.

> ❓ **OPEN:** Before I design the control layout I need the pan and tilt ranges in degrees
> and whether the two wheels need independent speeds.

> ❓ **OPEN:** Which drills matter for v1? (fixed interval feed, random pan/tilt within a
> window, programmed sequences, saved "shots").

## Safety requirements (not negotiable, designed in from the first commit)

- **Heartbeat:** the web app sends a ping every 500 ms. No ping for 1.5 s ⇒ wheels off,
  feeder parks, status "connection lost". Phone screen off, app backgrounded or Wi-Fi drop
  all lead here.
- **E-STOP** button always visible in the UI, on every screen, largest touch target. Wheels
  off at once (the MOSFET modules cannot brake, so the disks coast down), feeder parks, and
  an explicit "reset" is required to continue.
- **Parking after E-STOP** is Tilen's spec. It moves the carriage away from the wheels, at
  reduced speed, and it is the one motion that still happens after an E-STOP. If that ever
  becomes a pinch hazard at the tube end, a physical switch on the aim & feed rail is the way
  to freeze everything.
- **Soft start** on the wheels. Duty changes are rate-limited in firmware, not in the UI,
  and the second wheel starts a moment after the first, so the wheel rail never sees two
  spin-up currents at once.
- **Duty cap:** no code path may write more than `WHEEL_MAX_DUTY` to the wheel PWM. The
  wheel module clamps every request; the UI only ever sees 0–100 % of the cap.
- **Safe boot state:** hardware pull-ups/downs (see `wiring/pin-map.md`) plus firmware sets
  every output to "off" as the very first thing in `setup()`.
- **Feeder interlocks:** the carriage only moves to WHEELS when the wheels are at the
  requested speed (a shuttle pushed into stopped wheels jams the machine); homing only runs
  with the wheels off; an unexpected endstop trigger outside homing stops the feeder.
- **Soft limits** on pan and tilt once their limits are taught, and on the carriage once it is
  homed. Before that, only slow jog moves.
- **Hardware watchdog** enabled so a firmware hang resets the ESP32 into the safe state.
- Physical e-stop on the wheel rail as the last line (`wiring/05-power.md`).

## Web app v1: feature list

- Connection indicator (connected / reconnecting / lost) and round-trip latency.
- **E-STOP** (red, full width, sticky).
- Wheels: master speed slider 0–100 % (of the cap), START / STOP, optional left/right trim.
- Aim: pan/tilt jog pad (tap = small step, hold = continuous), numeric readout, and a
  "set limits" mode: jog to each end and confirm, as the prototype did with two buttons;
  limits persist in flash.
- Feed: "Feed one" button, auto-feed interval slider, START / STOP auto-feed, shot counter
  against the number of shuttles loaded.
- Feeder setup: jog the carriage and save PARK, TUBE and WHEELS; set and test the four servo
  angles and their settle times; everything stored in flash.
- Status line: wheel duty, pan/tilt position, feeder state, e-stop state, uptime.
- Mobile-first layout: portrait phone, thumb-reachable, big controls, works in landscape.

## Roadmap

1. Firmware skeleton in PlatformIO: AP + web server serving a "hello" page + WebSocket echo.
   Test from the phone.
2. Wheel control with heartbeat and e-stop (bare motor shafts, no disks), with
   `WHEEL_RAIL_V` matching the buck actually fitted.
3. Pan/tilt jog with acceleration; limit teach-in and soft limits.
4. Feeder: start-up and park routines, carriage homing, setup screen for positions and servo
   angles, then a single shot.
5. Auto-feed with interval; feeder interlock on wheel speed.
6. Drills, presets.
7. Nice-to-have: mDNS name (`launcher.local`), OTA updates over Wi-Fi, RPM feedback per
   wheel, empty-tube sensor.
