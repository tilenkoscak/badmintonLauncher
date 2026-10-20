# Web app (phone UI)

Not started. Planned as a **single `index.html`** with inline CSS and JavaScript, no
framework, no build step, no external resources.

Why single-file and offline: the phone is connected to the robot's own Wi-Fi access point,
which has no internet, so every byte the page needs must come from the ESP32. One file also
makes embedding into the firmware trivial (`../firmware/tools/embed_webapp.py`).

Constraints:

- Target: **Android phone, Brave browser** (Chromium engine; Tilen, 2026-10-20). iOS is not
  a target. Plain ES2017 JavaScript, no dependencies, so desktop Brave, Chrome or Edge work
  for development too.
- Mobile-first, portrait, thumb-reachable controls, minimum touch target 48 px.
- Reconnecting WebSocket client with a 500 ms heartbeat; UI greys out and shows "lost"
  when the socket drops.
- Keep the gzipped size well under 100 KB.

Opening the app on the phone:

- Join the robot's Wi-Fi. Android warns that the network has no internet: choose to stay
  connected. If the page still does not load, Android is sending the traffic over mobile
  data; switch mobile data off while using the robot.
- In Brave, open `http://192.168.4.1`, typed with `http://`. If Brave tries HTTPS and fails,
  turn off "Upgrade connections to HTTPS" in Brave Shields for this address. Then "Add to Home
  screen" for one-tap access.
- No captive-portal popup in v1 (the ESP32 does not redirect DNS), so the app always runs in
  Brave itself, not in Android's sign-in window.
- The Screen Wake Lock API needs HTTPS, so the page cannot keep the screen on by itself. Set
  the phone's screen timeout long enough for a session. When the screen does turn off, the
  heartbeat stops and the robot switches the wheels off and parks the feeder, by design.

For development the file can be opened directly in a desktop browser against a running
ESP32 (the WebSocket URL is derived from `location.host`, so it works both embedded and
served from disk with a hard-coded IP fallback).
