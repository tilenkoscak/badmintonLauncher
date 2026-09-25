# 06 · Optical endstop (from the Sidewinder X3 Plus)

One Artillery-style optical endstop board is available: a slotted infrared photo-interrupter,
an indicator LED and a 3-wire connector. Unlike a mechanical micro switch it is **powered**
and outputs an **active logic level**, so it must be treated as a small circuit, not as a
contact.

> ❓ **OPEN:** Before this page is final I need (see `PARTS.md` #13): a photo of the board or
> its pin labels and part numbers, whether it works when powered from 3.3 V, and what it is
> for: pan homing, tilt homing or shuttle detection. The pin in `pin-map.md` depends on that.

## How it behaves

| Mechanical micro switch | Optical endstop |
|---|---|
| Two wires, dry contact | Three wires: VCC, GND, SIGNAL |
| No power needed; ESP32 `INPUT_PULLUP` and switch to GND | Needs VCC. SIGNAL is driven by the board |
| Level defined by the wiring | Level defined by the board's circuit: SIGNAL flips when a flag enters the slot. Some boards go LOW when blocked, some HIGH |
| Wear, bounce | No wear, no bounce, very repeatable position |

The "different logic" Tilen noticed is this: the board itself decides which level means
"blocked", and it is active in both states. Which way round it is gets established once with
the test below and lands in the firmware as `LIMIT_ACTIVE_LEVEL`.

## The 3.3 V question

If the board is powered from 5 V, SIGNAL swings up to 5 V, which is **not allowed** into an
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
   │ SIG  ───────┼─────────────────────── GPIO 1 (pan) or GPIO 2 (tilt), INPUT (pull-up if the board is open-collector)
   └─────────────┘
```

| Board pin | Connect to | Notes |
|---|---|---|
| VCC (also marked V, +, 5V) | ESP32 `3V3` | Draws a few mA |
| GND (G, −) | ESP32 GND | |
| SIGNAL (S, OUT, D) | GPIO 1 or 2 per `pin-map.md` | Use `INPUT_PULLUP` if the level floats with nothing in the slot; otherwise plain `INPUT` |

## Smoke test: find the logic

```cpp
const int END_PIN = 1;   // pan endstop per pin-map.md

void setup() {
  Serial.begin(115200);
  pinMode(END_PIN, INPUT_PULLUP);
}

void loop() {
  Serial.printf("endstop = %d\n", digitalRead(END_PIN));
  delay(200);
}
```

Power the board from 3V3. Watch the serial monitor while sliding a piece of card into the
slot. Expected: the value flips reliably between 0 and 1 and the board's own LED changes.
Write down which value means "blocked": that is `LIMIT_ACTIVE_LEVEL`. If the value never
changes, the board does not work at 3.3 V: switch to plan B (5 V + divider). If it flips but
is noisy, add a 100 nF capacitor from SIGNAL to GND at the ESP32 end.

## Using it for homing

For pan or tilt, mount a small flag on the moving part so it enters the slot at one end of
travel. On start-up the firmware moves slowly toward that end until the endstop triggers,
sets position = 0 there, and backs off a few steps. Only after homing are the soft limits
active. The other axis needs a second endstop (optical or a mechanical micro switch on
`GPIO 2`, `INPUT_PULLUP`, switch to GND).
