# 06 · Optical endstop: the feeder's home sensor

One Artillery-style optical endstop board is available: a slotted infrared photo-interrupter,
an indicator LED and a 3-wire connector. On the Uno prototype it is the **feeder limiter**
(`controller/reference-uno/`): the feeder stepper homes against it at start-up, and the
code treats an unexpected trigger as an emergency stop. Unlike a mechanical micro switch it
is **powered** and outputs an **active logic level**, so it must be treated as a small
circuit, not as a contact.

| Signal | GPIO | Firmware constant |
|---|---|---|
| Endstop SIGNAL | GPIO 1 (`FEED_HOME`) | `FEED_HOME_ACTIVE_LEVEL = LOW` (from the Uno code; re-check on 3.3 V) |

> ❓ **OPEN:** Before this page is final I need (see `PARTS.md` #13): a photo of the board or
> its pin labels and part numbers, and the result of the 3.3 V test below. I also assume the
> "feeding limiter" in the Uno code *is* this optical board and not a mechanical switch;
> one word from Tilen confirms it.

## How it behaves

| Mechanical micro switch | Optical endstop |
|---|---|
| Two wires, dry contact | Three wires: VCC, GND, SIGNAL |
| No power needed; ESP32 `INPUT_PULLUP` and switch to GND | Needs VCC. SIGNAL is driven by the board |
| Level defined by the wiring | Level defined by the board's circuit. On the prototype: **LOW when the flag is in the slot**, HIGH otherwise, read with a plain `INPUT` and no pull-up |
| Wear, bounce | No wear, no bounce, very repeatable position |

The "different logic" Tilen noticed is this: the board itself drives the line in both states,
so it works without a pull-up and reads LOW when triggered.

## The 3.3 V question

On the Uno the board ran from 5 V, so its SIGNAL swung to 5 V. That is **not allowed** into an
ESP32 pin. Two ways out:

- **Plan A, power it from 3V3.** Most of these boards (IR LED with a series resistor, a
  photo-transistor and a pull-up, sometimes an LM393 comparator) work fine at 3.3 V; the
  IR LED just runs a little dimmer. Then SIGNAL is 0–3.3 V and goes straight to the GPIO.
- **Plan B, keep 5 V and divide SIGNAL.** 10 kΩ from SIGNAL to the GPIO and 20 kΩ from the
  GPIO to GND gives 3.3 V for a 5 V signal.

## Connections (plan A)

```
   Optical endstop board                 ESP32-S3
   ┌─────────────┐
   │ VCC  ───────┼─────────────────────── 3V3
   │ GND  ───────┼─────────────────────── GND
   │ SIG  ───────┼─────────────────────── GPIO 1  (FEED_HOME), pinMode INPUT
   └─────────────┘
```

| Board pin | Connect to | Notes |
|---|---|---|
| VCC (also marked V, +, 5V) | ESP32 `3V3` | Draws a few mA |
| GND (G, −) | ESP32 GND | |
| SIGNAL (S, OUT, D) | GPIO 1 | Plain `INPUT` as on the Uno. If the level floats with nothing in the slot, switch to `INPUT_PULLUP` |

## Smoke test: confirm the logic on 3.3 V

```cpp
const int FEED_HOME = 1;

void setup() {
  Serial.begin(115200);
  pinMode(FEED_HOME, INPUT);
}

void loop() {
  Serial.printf("feed home = %d\n", digitalRead(FEED_HOME));
  delay(200);
}
```

Power the board from 3V3. Watch the serial monitor while sliding a piece of card into the
slot. Expected: `1` with the slot open, `0` with the slot blocked, and the board's own LED
changes. That confirms `FEED_HOME_ACTIVE_LEVEL = LOW` on 3.3 V. If the value never changes,
the board does not work at 3.3 V: switch to plan B (5 V + divider). If it flips but is noisy,
add a 100 nF capacitor from SIGNAL to GND at the ESP32 end.

## How the firmware uses it

Same as the Uno code: at start-up drive the feeder slowly toward the endstop until it
triggers, back off until it releases, call that position 0, then retract 2.5 revolutions to
the ready position. During operation an unexpected trigger stops the feeder. Details and the
speeds that worked are in `controller/reference-uno/README.md`.

Pan and tilt do **not** need endstops for v1: the prototype taught their limits with two
buttons at every start, and the web app will do the same and remember them. GPIO 2 and 42
stay reserved in `pin-map.md` in case endstops are added later.
