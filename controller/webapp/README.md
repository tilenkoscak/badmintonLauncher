# Web app (phone UI)

Not started. Planned as a **single `index.html`** with inline CSS and JavaScript, no
framework, no build step, no external resources.

Why single-file and offline: the phone is connected to the robot's own Wi-Fi access point,
which has no internet, so every byte the page needs must come from the ESP32. One file also
makes embedding into the firmware trivial (`../firmware/tools/embed_webapp.py`).

Constraints:

- Mobile-first, portrait, thumb-reachable controls, minimum touch target 48 px.
- Works in Safari (iOS) and Chrome (Android). Plain ES2017 JavaScript, no dependencies.
- Reconnecting WebSocket client with a 500 ms heartbeat; UI greys out and shows "lost"
  when the socket drops.
- Keep the gzipped size well under 100 KB.

For development the file can be opened directly in a desktop browser against a running
ESP32 (the WebSocket URL is derived from `location.host`, so it works both embedded and
served from disk with a hard-coded IP fallback).
