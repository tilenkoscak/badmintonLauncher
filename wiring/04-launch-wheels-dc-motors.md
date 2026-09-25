# 04 · Launch wheels: two 775 DC motors on a 24 V PWM rail

Draft. The power chain is known from the prototype; a few details of how it is connected
still need confirming (marked below).

| Signal | GPIO | Notes |
|---|---|---|
| Wheel L PWM | GPIO 12 | to the PWM pin of MOSFET module L. **10 kΩ pull-down to GND** so the wheel is off while the ESP32 boots |
| Wheel R PWM | GPIO 13 | to the PWM pin of MOSFET module R, same pull-down |
| GND | any ESP32 GND | to the GND pin of both module headers (they are the same node as the modules' DC−) |

## The chain, per wheel

```
 buck 24 V ──► fuse 10 A ──► E83-004 ──► DC+ ┌──────────────────┐ OUT+ ──────────────┬───────────┬──────── 775 (+)
                              (series)       │  MOSFET module   │                    │           │
                                             │  15 A / 400 W    │              1.5KE30A       flyback
   star GND ────────────────────────────► DC− │  low-side switch │ OUT− ──────────────┴───────────┴──────── 775 (−)
                                             │                  │                    (cathode band     (Schottky,
                                             │  PWM   GND       │                     on the OUT+ side) cathode to OUT+)
                                             └───┬─────┬────────┘
                    ESP32 GPIO 12 / 13 ──────────┘     └──── ESP32 GND
                    (10 kΩ pull-down to GND)

 Buffer: 1000 µF / 50 V across DC+ / DC− of the module, not across the motor.
 Noise:  0.1 µF ceramic directly across the motor terminals.
```

| Part | Role | Notes |
|---|---|---|
| Buck 36 → 24 V | Wheel rail | Fixed 24 V output. Max ≈ 13 A total from the 36 V / 9.7 A PSU output |
| Fuse 10 A | Wiring protection | Automotive blade fuse, one per branch |
| E83-004 Schottky, 40 V / 60 A | Series blocking diode | Stops the spinning wheel (a generator when slowing) from pushing current back into the buck. Anode toward the buck, cathode toward the module. Dissipates 3–4 W at full power, heatsink it |
| MOSFET module 15 A / 400 W | PWM switch | **Low-side**: OUT+ is tied to DC+ inside the module, the MOSFETs switch OUT−. Trigger input accepts 3.3 V, so the ESP32 drives it directly. Heatsink above ~10 A |
| 1.5KE30A TVS | Clamp and flyback | Across the motor, cathode band on the OUT+ side. Reverse-biased at 24 V (stand-off 25.6 V), conducts forward when the MOSFET turns off and the motor's inductance pushes OUT− above OUT+ |
| Flyback Schottky (to add) | Freewheeling path | The TVS does this job today but is not made for a continuous PWM freewheeling current. A spare E83-004 across the motor (cathode to OUT+, anode to OUT−) takes the current; the TVS stays as a clamp |
| 1000 µF / 50 V | Buffer for the PWM current pulses | Belongs on the module **input** (DC+/DC−). Across the motor it would be charged and discharged at the PWM frequency, which cooks the capacitor and spikes the MOSFETs |

> ❓ **OPEN:** Before I mark this diagram as matching the prototype I need Tilen to confirm
> (see `PARTS.md` #8–#11): which E83-004 legs are used and that the anode faces the buck; that
> DC+ and OUT+ of a module show continuity; where the 1000 µF sits today; and that the TVS
> band points at OUT+.

## Why the duty cycle is capped

The motors are rated **12 V**, the rail is **24 V**. Average motor voltage = duty × 24 V, so
100 % duty would run them at twice their rating (about 20 000 rpm; brushes, bearings and the
disks are not made for that). The firmware therefore has a hard constant

```
WHEEL_MAX_DUTY = 50 %      // 12 V average on a 24 V rail
```

that the UI cannot exceed. The UI's "100 % speed" maps to this cap. If the buck is ever
replaced by a 12 V one, the cap becomes 100 % and nothing else changes. Running at 24 V with
a duty cap is a legitimate way to get a fast spin-up: the motor sees full 24 V pulses at low
duty, so torque at low speed is higher than it would be on 12 V.

> ❓ **OPEN:** Before I fix the value I need the highest `analogWrite` value the Uno prototype
> used for a normal shot (0–255), and the buck output voltage measured under load.

## Direction

The MOSFET module switches one direction only. The two disks counter-rotate, so the leads of
one motor are swapped. Mark the wheels L and R and the motor leads + / − once and for all.

## ESP32 side

- PWM comes from the LEDC peripheral. The module accepts 0–20 kHz. The Uno drove it at its
  `analogWrite` default (490 Hz or 980 Hz), which is proven but whines. Start with **1 kHz**
  to reproduce the prototype, then raise to **10–16 kHz** and check the module's heatsink
  temperature after a minute at working duty; stay at or below 20 kHz.
  The frequency is one constant in firmware (`WHEEL_PWM_HZ`), 10-bit resolution (0–1023).
- One PWM line per wheel so left and right can differ later. If they must always match, the
  firmware writes the same duty to both.
- Pull-downs on the PWM lines: during reset the ESP32 pins float, and a floating trigger
  input can turn the module on.
- The module is not isolated: its header GND is the motor supply ground. Run the ESP32's
  ground wire to the star point, not to the module's DC− screw terminal, so the wheel current
  does not flow through the thin signal ground.
- OUT− is **not** ground. When the module is off, the whole motor sits at +24 V. Never tie
  OUT− or a motor terminal to the ESP32 GND.

## Wiring rules for the power side

- Motor and rail wires: ≥ 1.5 mm², twisted pair per motor, short, away from the ESP32, its
  antenna and the stepper/servo signal wires.
- 0.1 µF ceramic across each motor's terminals against brush noise.
- Both E83-004 and both modules on heatsinks. The diode tabs are cathodes: insulating washers
  if they share a heatsink.
- All power grounds straight to the star point (`05-power.md`).

## Safety, non-negotiable

- **No disks mounted** for the first power tests. Bare shafts only.
- The web app's E-STOP and the connection watchdog (`controller/README.md`) must be working
  before the disks go back on.
- Firmware ramps duty up over 1–2 s, starts the second wheel a moment after the first, and
  never jumps to a high duty.
- Physical E-STOP / wheel switch in the 24 V wheel rail, reachable without a hand near the
  wheels.
- Disks fully guarded so a shuttle or a finger cannot enter from the side.

## Smoke test

Wheel rail on, ESP32 on USB, common GND, motor with bare shaft. Ramps wheel L to 25 % duty
(≈ 6 V average), holds 2 s, stops 3 s. Core 3.x LEDC API (core 2.x: `ledcSetup` +
`ledcAttachPin` + `ledcWrite(channel, duty)`).

```cpp
const int WHEEL_L_PWM = 12;
const int WHEEL_PWM_HZ = 1000;   // prototype-like; raise to 10–16 kHz later
const int TEST_MAX_DUTY = 256;   // 25 % of 1023

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

Expected: smooth ramp, module and series diode barely warm, no reset of the ESP32. Motor
jumps to full speed = wrong pin or floating trigger (check the pull-down). Nothing = wheel
switch off, fuse, or missing ground between ESP32 and module header.

## Later improvements (not for v1)

- RPM sensor per wheel (Hall sensor + magnet, or optical) and a PI loop, so a given "speed"
  gives the same launch distance regardless of rail voltage sag.
- A 12 V buck variant to remove the duty cap, or a proper H-bridge / ESC if braking or
  reversing is ever wanted.
