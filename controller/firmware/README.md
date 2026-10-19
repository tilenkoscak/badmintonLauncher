# Firmware (ESP32-S3)

Not started. It will be a **PlatformIO** project (decided 2026-10-05), Arduino framework,
laid out like Tilen's Uno project in `../reference-uno/ShuttlecockLauncher/`:

```
firmware/
  platformio.ini            board config from wiring/01-esp32-s3-basics.md. Env "launcher" builds
                            the real firmware; one extra env per smoke test (build_src_filter),
                            as in the Uno project
  include/
    config.h                every GPIO from wiring/pin-map.md, Wi-Fi SSID/password, limits,
                            WHEEL_RAIL_V (24 until the 12 V buck is fitted) and the derived
                            WHEEL_MAX_DUTY, WHEEL_PWM_HZ, FEED_HOME_ACTIVE_LEVEL (LOW),
                            FEED_HOME_DIR, servo pulse range 500–2500 µs, default servo angles
                            and settle times, microstep multipliers per axis
    web_index.h             generated: gzipped ../webapp/index.html as a byte array
  src/
    main.cpp                setup()/loop(): boot to safe state, start Wi-Fi + server, run tasks
    safety.cpp/.h           heartbeat watchdog, e-stop, safe-state function
    wheels.cpp/.h           LEDC PWM for the two MOSFET modules, rate-limited ramps, duty cap
    aim.cpp/.h              pan/tilt steppers: jog, move-to, limit teach-in, soft limits
    feeder.cpp/.h           carriage homing, park routine and the feed cycle as a state machine
                            (../README.md, "Feeder state machine")
    settings.cpp/.h         taught limits, carriage positions and servo angles in flash (Preferences)
    net.cpp/.h              access point, HTTP server, WebSocket, JSON in/out
    smoke/                  the smoke tests from wiring/, one file each, excluded from "launcher"
  tools/
    embed_webapp.py         PlatformIO pre-build script (extra_scripts = pre:...), writes include/web_index.h
```

Design rules for the firmware:

- Motion runs on core 1 (`loop()`), networking on core 0 (Wi-Fi stack + async server).
  Nothing in `loop()` may block for more than a few ms; no `delay()` in motion code. The
  servo settle times are timers in the feeder state machine, not `delay()` calls.
- All outputs are set to their safe state before anything else in `setup()`.
- Pin numbers live only in `config.h`.
- Start values for speeds and homing come from `../reference-uno/README.md`, not from the raw
  Uno constants: the Uno could not reach its commanded feeder speed, and its pan/tilt
  microstep multiplier does not match the wiring. The servo angles and the 2.5-turn stroke in
  that README belong to the old feeder; the redesigned feeder's values are taught in the app.
