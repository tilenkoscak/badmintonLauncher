# 04 · Launch wheels: two 775 DC motors on the 12 V PWM rail

Draft. The wheel chain is known and matches the prototype: buck → fuse → MOSFET module with
a 1000 µF buffer and a TVS on its input → motor with a Schottky flyback diode across it.
The rail is becoming **12 V**: a 36/48 V → 12 V, 30 A buck is ordered and replaces the
prototype's 24 V one. Until it is fitted, see "Until the new buck arrives" below.

| Signal | GPIO | Notes |
|---|---|---|
| Wheel L PWM | GPIO 12 | to the PWM pin of MOSFET module L. **10 kΩ pull-down to GND** so the wheel is off while the ESP32 boots |
| Wheel R PWM | GPIO 13 | to the PWM pin of MOSFET module R, same pull-down |
| GND | any ESP32 GND | to the GND pin of both module headers (they are the same node as the modules' DC−) |

## The chain, per wheel

```
 buck output ──► fuse ──┬──────────┬──────► DC+ ┌──────────────────┐ OUT+ ─────────────┬────────── 775 (+)
 (12 V rail)            │          │            │  MOSFET module   │                   │
                     1000 µF   1.5KE30A         │  15 A / 400 W    │              E83-004 Schottky
                      50 V     band to DC+      │  low-side switch │              cathode to OUT+
                        │          │            │                  │              anode to OUT−
 star GND ──────────────┴──────────┴──────► DC− │  PWM   GND       │ OUT− ─────────────┴────────── 775 (−)
                                                └───┬─────┬────────┘
                       ESP32 GPIO 12 / 13 ──────────┘     └──── ESP32 GND
                       (10 kΩ pull-down to GND)

 Add: 0.1 µF ceramic directly across the motor terminals (brush noise).
```

| Part | Position | Role |
|---|---|---|
| Buck 36 V → 12 V, 30 A (ordered; the 24 V one until it arrives) | Feeds both branches | Fixed-output IP68 module. The 36 V / 9.7 A PSU output behind it allows ≈ 26 A at 12 V for both wheels together |
| Fuse | In series, one per branch | 15 A automotive blade. Wiring protection |
| 1000 µF / 50 V | Across DC+ / DC− of the module | Buffers the PWM current pulses so the buck and the wiring see a smooth current |
| 1.5KE30A TVS | Across DC+ / DC− of the module, band to DC+ | Clamps rail transients (switching spikes, buck overshoot) before they reach the module. Stand-off 25.6 V: comfortable margin on 12 V |
| MOSFET module 15 A / 400 W | Between rail and motor | **Low-side**: OUT+ is tied to DC+ inside the module, the MOSFETs switch OUT−. Trigger input accepts 3.3 V, so the ESP32 drives it directly. Heatsink it: steady current is low, spin-up peaks are not |
| E83-004 Schottky, 40 V / 60 A | Across OUT+ / OUT−, cathode to OUT+ | **Flyback diode.** When the MOSFET turns off, the motor's inductance keeps current flowing; this diode gives it a path. Carries the motor current during the off-time only, ≈ 1–2 W at full power, a small heatsink or none |
| 775 motor | OUT+ / OUT− | 12 V, 150 W. Leads swapped on one wheel for counter-rotation |

## Why 12 V

The motors are rated **12 V**. On the prototype's 24 V rail the average motor voltage is
duty × 24 V, so 100 % duty would run them at twice their rating (about 20 000 rpm, four times
the rim stress in the disks), and only a firmware constant prevented it. On the 12 V rail the
hardware limits the speed to the rated 10 000 rpm, the whole duty range is usable, and the
brushes see 12 V pulses instead of 24 V ones (less arcing, heat and noise).

The firmware still carries the cap, derived from the rail voltage it is told:

```
WHEEL_RAIL_V   = 24                               // set to 12 once the new buck is fitted and measured
WHEEL_MAX_DUTY = min(100 %, 12 V / WHEEL_RAIL_V)  // 50 % on 24 V, 100 % on 12 V
```

It ships with 24, so the firmware is safe on whichever buck is fitted. The UI's "100 % speed"
maps to the cap.

Current ripple grows with rail voltage and shrinks with PWM frequency. At the Uno's ~1 kHz a
low-inductance 775 sees a large ripple current (heat, brush arcing, noise); at 16 kHz it is
small. So the modules run near their 20 kHz limit.

Current draw on 12 V: at steady speed a flywheel needs only a few amps (bearing and air
losses). The peaks come at spin-up, when a 775 pulls tens of amps for a moment. PSU output 2
delivers about 26 A at 12 V for both wheels together, which is why the firmware ramps over
1–2 s and starts the second wheel after the first.

What the Uno code tells us (`controller/reference-uno/README.md`): the only wheel program is
a single-motor test at 980 Hz that kicks at duty 255 for 200 ms, drops to duty 100 and ramps
to 255 and back. So a 775 has already seen full duty on the 24 V rail, at least without a
shuttle. No shot-tuning values were recorded.

> ❓ **OPEN:** If shots were ever tuned by hand on the prototype, I need the duty (0–255 on
> the 24 V rail) that gave a good shot. Above 128 is out of reach on 12 V, which would reopen
> the rail question.

## Until the new buck arrives

- The 24 V buck stays in, and the firmware keeps `WHEEL_RAIL_V = 24`.
- Every wheel test stays at or below 50 % duty. The smoke test below uses 25 %.
- When the 12 V buck arrives: fit it, measure its output without load and then with one
  wheel at 50 % duty (expect 12 V ± 0.5 V), fit 15 A fuses and 2.5 mm² wire, and only then
  set `WHEEL_RAIL_V = 12`.

## Direction

The MOSFET module switches one direction only. The two disks counter-rotate, so the leads of
one motor are swapped. Mark the wheels L and R and the motor leads + / − once and for all.

## ESP32 side

- PWM comes from the LEDC peripheral, 10-bit resolution (0–1023). The module accepts
  0–20 kHz. Target **16 kHz** (`WHEEL_PWM_HZ`), which keeps current ripple small on either
  rail. If the module's heatsink runs hot at working duty, drop to 10 kHz. The Uno drove the
  module at 980 Hz, which proves the module but whines and ripples; use it only to reproduce
  the prototype on the first bring-up.
- The Uno test used a 200 ms full-duty kick to overcome stall. Keep that as an option in
  firmware (`WHEEL_KICK_MS`), but on a 12 V rail a plain ramp is probably enough, and on the
  24 V rail a kick must respect `WHEEL_MAX_DUTY`.
- One PWM line per wheel so left and right can differ later. If they must always match, the
  firmware writes the same duty to both.
- Pull-downs on the PWM lines: during reset the ESP32 pins float, and a floating trigger
  input can turn the module on.
- The module is not isolated: its header GND is the motor supply ground. Run the ESP32's
  ground wire to the star point, not to the module's DC− screw terminal, so the wheel current
  does not flow through the thin signal ground.
- OUT− is **not** ground. When the module is off, the whole motor sits at rail voltage.
  Never tie OUT− or a motor terminal to the ESP32 GND.

## Wiring rules for the power side

- Motor and rail wires: 2.5 mm², twisted pair per motor, short,
  away from the ESP32, its antenna and the stepper/servo signal wires.
- 0.1 µF ceramic across each motor's terminals against brush noise.
- Both modules on heatsinks. The E83-004 tabs are cathodes: insulating washers if they share
  a heatsink with anything else.
- Keep the 1000 µF and the TVS right at the module's screw terminals, short leads.
- All power grounds straight to the star point (`05-power.md`).

## Safety, non-negotiable

- **No disks mounted** for the first power tests. Bare shafts only.
- The web app's E-STOP and the connection watchdog (`controller/README.md`) must be working
  before the disks go back on.
- Firmware ramps duty up over 1–2 s, starts the second wheel a moment after the first, and
  never jumps to a high duty.
- Physical E-STOP / wheel switch in the wheel rail, reachable without a hand near the wheels.
- Disks fully guarded so a shuttle or a finger cannot enter from the side.

## Smoke test

Wheel rail on, ESP32 on USB, common GND, motor with bare shaft. Ramps wheel L to 25 % duty
(≈ 6 V average on the 24 V rail, 3 V on 12 V), holds 2 s, stops 3 s. Core 3.x LEDC API
(core 2.x: `ledcSetup` + `ledcAttachPin` + `ledcWrite(channel, duty)`).

```cpp
#include <Arduino.h>

const int WHEEL_L_PWM = 12;
const int WHEEL_PWM_HZ = 16000;  // module limit is 20 kHz; drop to 10 kHz if it runs hot
const int TEST_MAX_DUTY = 256;   // 25 % of 1023, safe on either rail

void setup() {
  ledcAttach(WHEEL_L_PWM, WHEEL_PWM_HZ, 10);   // pin, frequency, 10-bit
  ledcWrite(WHEEL_L_PWM, 0);
}

void loop() {
  for (int d = 0; d <= TEST_MAX_DUTY; d += 4) { ledcWrite(WHEEL_L_PWM, d); delay(20); }  // ~1.3 s ramp
  delay(2000);
  ledcWrite(WHEEL_L_PWM, 0);
  delay(3000);
}
```

Expected: smooth ramp, module and flyback diode barely warm, no reset of the ESP32. Motor
jumps to full speed = wrong pin or floating trigger (check the pull-down). Nothing = wheel
switch off, fuse, or missing ground between ESP32 and module header.

## Later improvements (not for v1)

- RPM sensor per wheel (Hall sensor + magnet, or optical) and a PI loop, so a given "speed"
  gives the same launch distance regardless of rail voltage sag.
- Current sensing per wheel: a 775 on 12 V can stall at tens of amps if a shuttle jams, and
  the MOSFET module has no protection of its own.
- A proper H-bridge / ESC if braking or reversing is ever wanted.
