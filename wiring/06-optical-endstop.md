# 06 · Optical endstop: the feeder's home sensor

One Artillery-style optical endstop board is available: a slotted infrared photo-interrupter,
an indicator LED and a 3-wire connector. On the Uno prototype it is the **feeder limiter**
(`controller/reference-uno/`): the feeder stepper homes against it at start-up (in the redesigned feeder: the carriage), and the
code treats an unexpected trigger as an emergency stop. Unlike a mechanical micro switch it
is **powered** and outputs an **active logic level**, so it must be treated as a small
circuit, not as a contact.

| Signal | GPIO | Firmware constant |
|---|---|---|
| Endstop SIGNAL | GPIO 1 (`FEED_HOME`) | `FEED_HOME_ACTIVE_LEVEL = LOW` (from the Uno code; re-check on 3.3 V) |
| Homing direction | - | `FEED_HOME_DIR`: towards the wheels |

**Position (Tilen, 2026-10-20):** at the **front** of the carriage travel, towards the wheels.
It marks the furthest the feeding mechanism may move: far enough that the shuttle is fed into
the flywheels, not so far that the mechanism runs into them. This is the Uno code's "feeding
limiter".

**Operating voltage:** not published. A web search (2026-10-20) found no datasheet or
schematic for the Sidewinder X3 Plus endstop or for the endstop connector on its mainboard. That
mainboard uses an STM32F401, a 3.3 V chip, but that alone does not show whether it fed the
endstop 3.3 V or 5 V. The quick test below answers it in five minutes.

> ❓ **OPEN:** Before this page is final I need the result of the quick 3.3 V test below (a
> multimeter and the Uno are enough): does SIGNAL swing between about 3.3 V and 0 V when a
> card goes into the slot? A photo of the endstop board's components would also let me check
> its circuit, but the test is the faster answer.

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

Running a board on 3.3 V that was designed for 5 V cannot damage it; the worst case is that
it does not switch. So the test is safe.

### Quick 3.3 V test, no ESP32 needed

The Uno has a `3.3V` pin, which is enough for this board.

1. Unplug the endstop from everything else. Uno on USB.
2. Endstop VCC → Uno `3.3V`, endstop GND → Uno `GND`. SIGNAL connects to nothing.
3. Multimeter on DC volts: black probe on GND, red probe on SIGNAL.
4. Slot open: note the reading (expected about 3.3 V). Slide a piece of card into the slot:
   note the reading again (expected about 0 V). Watch the board's LED too.

Two clearly different readings (for example 3.2 V and 0.1 V) mean plan A works; send me both
numbers. If the reading does not change, or the LED stays dark, the board needs 5 V: plan B.

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
#include <Arduino.h>

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

It homes the feeder carriage, as on the Uno: at start-up, after the arms are opened and the
lift is lowered, drive the carriage slowly **towards the wheels** until the endstop triggers,
back off until it releases and call that position 0. PARK, TUBE and WHEELS are counted in
microsteps back from there (`controller/README.md`, "Feeder state machine"); WHEELS sits at 0
or just behind it. Because the homing move runs towards the wheels, homing only runs with the
wheels off. During operation an unexpected trigger stops the feeder. The homing speeds that
worked on the Uno are in `controller/reference-uno/README.md`.

Pan and tilt have **no** endstops: their START and END are taught in the app, and after each
power-up the user confirms START at a mark on the frame (`controller/README.md`). GPIO 2 and
42 stay reserved in `pin-map.md` in case home sensors are ever added.
