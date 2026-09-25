# Controller: ESP32-S3 firmware + phone web app

Goal: one controller that runs the whole launcher, operated from a phone browser.
No code exists yet. This file records the planned architecture and the open decisions so
the code can be built iteratively, one feature at a time.

## Layout

```
controller/
  README.md          this plan
  firmware/          Arduino sketch for the ESP32-S3           (to be created)
  webapp/            HTML/CSS/JS of the phone UI                (to be created)
  reference-uno/     Tilen's prototype sketches for the Uno     (to be added by Tilen)
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
                                              │      ⇒ wheels off, feeder stop          │
                                              │  motion: steppers (pan/tilt/feed),      │
                                              │          servos, wheel PWM              │
                                              └─────────────────────────────────────────┘
```

1. The ESP32 runs as a **Wi-Fi access point**. The phone joins it and opens
   `http://192.168.4.1`. No router, no internet, works on the court.
2. The ESP32 **serves the web app** (one self-contained HTML file embedded in the firmware).
3. The web app and the firmware talk over a **WebSocket**: JSON commands from the phone,
   JSON status back at 5–10 Hz (positions, wheel duty, shuttle count, e-stop state, battery).
4. The firmware owns all motion and all safety logic. The phone is only a remote.

## Decisions taken

| Topic | Decision | Why |
|---|---|---|
| Link | Wi-Fi, ESP32 as access point. Optionally also join the home network during development | Works on iPhone and Android, no pairing, no app install. Bluetooth is not reachable from iOS browsers and Web Bluetooth is Chrome-only |
| Protocol | WebSocket carrying small JSON messages | Bidirectional, low latency, trivial in browser JS, easy to debug |
| UI delivery | Single `index.html` (CSS and JS inline), gzipped and embedded in the firmware as a byte array by a small build script | One upload, no filesystem-upload plugin needed, 16 MB flash is plenty |
| No external resources in the UI | Everything inline, no CDN, no web fonts | The phone has **no internet** while connected to the robot |
| Framework | Arduino (arduino-esp32 core 3.x) | Tilen already knows Arduino |
| Libraries (candidates) | `ESPAsyncWebServer` + `AsyncTCP` (ESP32Async forks) for HTTP + WebSocket · `FastAccelStepper` (hardware-timed) or `AccelStepper` for steppers · `ESP32Servo` · built-in LEDC for wheel PWM | To be confirmed when the firmware skeleton is started |
| Wheel duty cap | `WHEEL_MAX_DUTY` ≈ 50 % while the wheel rail is 24 V, 100 % once a 12 V buck is fitted. A compile-time constant the UI cannot exceed; UI "100 %" maps to the cap | The 775 motors are 12 V (`wiring/04-launch-wheels-dc-motors.md`) |
| Wheel PWM frequency | `WHEEL_PWM_HZ` ≈ 16 kHz (10 kHz if the modules run hot), never above 20 kHz | Limit of the MOSFET modules; high frequency keeps current ripple low |
| Homing | Optical endstop (from the printer) on one axis, second endstop to be added; active level is a constant `LIMIT_ACTIVE_LEVEL` | `wiring/06-optical-endstop.md` |

## Decisions still open

> ❓ **OPEN:** Before I start the firmware skeleton I need to know: Arduino IDE 2.x or
> PlatformIO in VS Code? (Arduino IDE = what Tilen knows; PlatformIO = better library
> pinning and the embed-the-webapp build step is one line of config.)

> ❓ **OPEN:** Before I write the web app I need to know which phone(s) it must run on
> (Android, iPhone, both) and the browser. iOS Safari behaves differently on access points
> without internet (captive-portal popup), which affects how the app is opened.

> ❓ **OPEN:** Before I design the control layout I need the pan and tilt ranges in degrees,
> which axis gets the existing optical endstop and whether a second one will be fitted
> (`wiring/pin-map.md` reserves GPIO 1/2), and whether the two wheels need independent speeds.

> ❓ **OPEN:** Before I write the feeder state machine I need the exact feed sequence from the
> prototype: order and timing of servo A, servo B and the feeder stepper for one shuttle,
> and what "magazine empty" looks like.

> ❓ **OPEN:** Which drills matter for v1? (fixed interval feed, random pan/tilt within a
> window, programmed sequences, saved "shots").

## Safety requirements (not negotiable, designed in from the first commit)

- **Heartbeat:** the web app sends a ping every 500 ms. No ping for 1.5 s ⇒ wheels ramp to
  zero, feeder stops, status "connection lost". Phone screen off, app backgrounded or
  Wi-Fi drop all lead here.
- **E-STOP** button always visible in the UI, on every screen, largest touch target. Stops
  everything immediately and requires an explicit "reset" to continue.
- **Soft start / soft stop** on the wheels. Duty changes are rate-limited in firmware, not
  in the UI, and the second wheel starts a moment after the first so the 24 V rail never
  sees two spin-up currents at once.
- **Duty cap:** no code path may write more than `WHEEL_MAX_DUTY` to the wheel PWM. The
  wheel module clamps every request; the UI only ever sees 0–100 % of the cap.
- **Safe boot state:** hardware pull-ups/downs (see `wiring/pin-map.md`) plus firmware sets
  every output to "off" as the very first thing in `setup()`.
- **Feeder interlock:** the feeder only cycles when the wheels are at the requested speed.
  A shuttle dropped into stopped wheels jams the machine.
- **Soft limits** on pan and tilt after homing. Before homing, only slow jog moves.
- **Hardware watchdog** enabled so a firmware hang resets the ESP32 into the safe state.
- Physical e-stop on the motor rail as the last line (`wiring/05-power.md`).

## Web app v1: feature list

- Connection indicator (connected / reconnecting / lost) and round-trip latency.
- **E-STOP** (red, full width, sticky).
- Wheels: master speed slider 0–100 %, START / STOP, optional left/right trim.
- Aim: pan/tilt jog pad (tap = small step, hold = continuous), "home", numeric readout.
- Feed: "Feed one" button, auto-feed interval slider (e.g. 1–10 s), START / STOP auto-feed,
  shuttle counter.
- Status line: wheel duty, pan/tilt position, e-stop state, uptime, (battery later).
- Mobile-first layout: portrait phone, thumb-reachable, big controls, works in landscape.

## Roadmap

1. Firmware skeleton: AP + web server serving a "hello" page + WebSocket echo. Test from the phone.
2. Wheel control with heartbeat and e-stop (bare motor shafts, no disks).
3. Pan/tilt jog with acceleration; homing if switches exist.
4. Feeder sequence, single shot.
5. Auto-feed with interval; feeder interlock on wheel speed.
6. Drills, presets, persistence of settings in flash.
7. Nice-to-have: mDNS name (`launcher.local`), OTA updates over Wi-Fi, battery display,
   RPM feedback.
