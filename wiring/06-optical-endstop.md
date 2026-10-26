# 06 · Optical endstop: the feeder's home sensor

The sensor salvaged from the Sidewinder X3 Plus. On the Uno prototype it is the **feeder
limiter** (`controller/reference-uno/`): the feeder stepper homes against it at start-up (in
the redesigned feeder: the carriage), and the code treats an unexpected trigger as an
emergency stop. Unlike a mechanical micro switch it is **powered** and outputs an **active
logic level**, so it must be treated as a small circuit, not as a contact.

**What it really is (photo, 2026-10-23): not a slotted optical endstop.** The part is a small
black sensor body with a **round sensing face** at one end, a mounting hole, an indicator LED
and a three-wire cable. That matches the X3 Plus's **inductive bed-levelling sensor** (Artillery
sells it as "Inductive Levelling Sensor for SW-X3/SW-X4"), a proximity sensor that reacts to
**metal, steel best, a few millimetres in front of its round face**. Cardboard or pressing on
it does nothing, which explains why none of the bench attempts switched. The page keeps the
name "optical endstop" until the steel test below confirms this. If it is confirmed, the
redesigned carriage needs a piece of steel (a screw head, a washer, a strip of sheet steel)
that passes within about 1-2 mm of the round face at the home position.

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

> ❓ **OPEN:** Before I rewrite this page I need the bench test below done with a **steel
> object** instead of a card (drawing in `wiring-diagrams.html`, section 7): the SIGNAL
> voltage with nothing near the round face and with steel against it, first with the sensor
> on 3.3 V, then on 5 V. Also the three wire colours, and which part of the prototype's
> carriage came close to the round face (it should be metal). Earlier attempts (2026-10-21/22)
> used a card and never switched; the first wire order gave about 70 % of the supply at rest,
> which is what a correctly powered sensor of this kind shows.

## How it behaves

| Mechanical micro switch | This sensor |
|---|---|
| Two wires, dry contact | Three wires: VCC, GND, SIGNAL |
| No power needed; ESP32 `INPUT_PULLUP` and switch to GND | Needs VCC. SIGNAL is driven by the board |
| Level defined by the wiring | Level defined by the sensor's circuit. On the prototype: **LOW when triggered**, HIGH otherwise, read with a plain `INPUT` and no pull-up |
| Wear, bounce | No wear, no bounce, very repeatable position |

The "different logic" Tilen noticed is this: the board itself drives the line in both states,
so it works without a pull-up and reads LOW when triggered.

## The 3.3 V question

On the Uno the board ran from 5 V, so its SIGNAL swung to 5 V. That is **not allowed** into an
ESP32 pin. Two ways out:

- **Plan A, power it from 3V3.** Then SIGNAL is 0-3.3 V and goes straight to the GPIO.
  Inductive sensors often need 5 V or more to work, so plan A may well fail; the test tells.
- **Plan B, keep 5 V and divide SIGNAL.** 10 kΩ from SIGNAL to the GPIO and 20 kΩ from the
  GPIO to GND gives 3.3 V for a 5 V signal.

Running a sensor on 3.3 V that was designed for 5 V cannot damage it; the worst case is that
it does not switch. So the test is safe.

### Bench test with the Uno and a multimeter, no ESP32 needed

Drawing: `wiring-diagrams.html`, section 7. Wire colours below follow the usual sensor code
(brown +, blue -, black signal); if yours differ, use the wire order of the first attempt,
which gave about 2.4 V at rest.

1. Unplug the sensor from everything else. Uno on USB.
2. Brown (VCC) → Uno `3.3V`, blue (GND) → Uno `GND`. Black (SIGNAL) connects to nothing.
3. Multimeter on DC volts, 20 V range, leads in the **COM** and **V** jacks. Black probe on the
   Uno's other `GND` pin, red probe on the black wire. The meter's COM, the sensor's GND and
   the Uno must share the same ground; that is the only "common ground" this test needs.
4. Nothing near the round face: note the reading. Hold a **steel object** (screwdriver blade,
   steel washer, steel ruler) flat against the round face: note the reading again. Watch the
   sensor's LED too.
5. Repeat with brown on the Uno's `5V` instead of `3.3V`.

Two clearly different readings (for example 2.4 V and 0.1 V) mean the sensor works at that
voltage; send me all four numbers. If nothing changes even at 5 V with steel touching the
face, tell me; then the next step is a closer look at the sensor itself.

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
| SIGNAL (S, OUT, D) | GPIO 1 | Plain `INPUT` as on the Uno. If the level floats with nothing in front of the sensor, switch to `INPUT_PULLUP` |

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

Power the sensor from 3V3. Watch the serial monitor while holding a steel object against the
round face. Expected: `1` with nothing in front of it, `0` with steel against it, and the
sensor's own LED changes. That confirms `FEED_HOME_ACTIVE_LEVEL = LOW` on 3.3 V. If the value never changes,
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
