# Firmware (ESP32-S3)

Not started. Planned structure once the open questions in `../README.md` are answered:

```
firmware/
  launcher/                 Arduino sketch folder (name = .ino name)
    launcher.ino            setup()/loop(): boot to safe state, start Wi-Fi + server, run tasks
    config.h                every GPIO from wiring/pin-map.md, Wi-Fi SSID/password, limits,
                            WHEEL_MAX_DUTY (≈ 50 %: 24 V rail, 12 V motors), WHEEL_PWM_HZ,
                            LIMIT_ACTIVE_LEVEL of the optical endstop
    safety.cpp/.h           heartbeat watchdog, e-stop, safe-state function
    wheels.cpp/.h           LEDC PWM for the two 775 drivers, rate-limited ramps
    aim.cpp/.h              pan/tilt steppers: jog, move-to, homing, soft limits
    feeder.cpp/.h           servo + stepper feed sequence as a state machine
    net.cpp/.h              access point, HTTP server, WebSocket, JSON in/out
    web_index.h             generated: gzipped ../webapp/index.html as a byte array
  tools/
    embed_webapp.py         builds web_index.h from ../webapp/index.html
```

Board settings for the Arduino IDE are in `../../wiring/01-esp32-s3-basics.md`.

Design rules for the firmware:

- Motion runs on core 1 (`loop()`), networking on core 0 (Wi-Fi stack + async server).
  Nothing in `loop()` may block for more than a few ms; no `delay()` in motion code.
- All outputs are set to their safe state before anything else in `setup()`.
- Pin numbers live only in `config.h`.
