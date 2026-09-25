# 04 · Launch wheels: two 775 DC motors on a PWM rail

Draft. The wheel chain is known and matches the prototype: buck → fuse → MOSFET module with
a 1000 µF buffer and a TVS on its input → motor with a Schottky flyback diode across it.
The one open decision is the rail voltage: **24 V today, 12 V recommended**.

| Signal | GPIO | Notes |
|---|---|---|
| Wheel L PWM | GPIO 12 | to the PWM pin of MOSFET module L. **10 kΩ pull-down to GND** so the wheel is off while the ESP32 boots |
| Wheel R PWM | GPIO 13 | to the PWM pin of MOSFET module R, same pull-down |
| GND | any ESP32 GND | to the GND pin of both module headers (they are the same node as the modules' DC−) |

## The chain, per wheel

```
 buck output ──► fuse ──┬──────────┬──────► DC+ ┌──────────────────┐ OUT+ ─────────────┬────────── 775 (+)
 (24 V today,           │          │            │  MOSFET module   │                   │
  12 V recommended)  1000 µF   1.5KE30A         │  15 A / 400 W    │              E83-004 Schottky
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
| Buck 36 V → 24 V (or 12 V) | Feeds both branches | Fixed-output IP68 module. The 36 V / 9.7 A PSU output behind it allows ≈ 13 A at 24 V or ≈ 26 A at 12 V |
| Fuse | In series, one per branch | 10 A automotive blade on 24 V, 15 A on 12 V. Wiring protection |
| 1000 µF / 50 V | Across DC+ / DC− of the module | Buffers the PWM current pulses so the buck and the wiring see a smooth current |
| 1.5KE30A TVS | Across DC+ / DC− of the module, band to DC+ | Clamps rail transients (switching spikes, buck overshoot) before they reach the module. Stand-off 25.6 V, breakdown ≥ 28.5 V |
| MOSFET module 15 A / 400 W | Between rail and motor | **Low-side**: OUT+ is tied to DC+ inside the module, the MOSFETs switch OUT−. Trigger input accepts 3.3 V, so the ESP32 drives it directly. Heatsink above ~10 A |
| E83-004 Schottky, 40 V / 60 A | Across OUT+ / OUT−, cathode to OUT+ | **Flyback diode.** When the MOSFET turns off, the motor's inductance keeps current flowing; this diode gives it a path. Carries the motor current during the off-time only, ≈ 1–2 W at full power, a small heatsink or none |
| 775 motor | OUT+ / OUT− | 12 V, 150 W. Leads swapped on one wheel for counter-rotation |

## Rail voltage: 24 V today, 12 V recommended

The motors are rated **12 V**. On the 24 V rail, average motor voltage = duty × 24 V, so
100 % duty would run them at twice their rating (about 20 000 rpm; four times the rim stress
in the disks). Two ways to handle it:

| | 24 V rail (as is) | 12 V rail (recommended) |
|---|---|---|
| Speed limit | Firmware constant `WHEEL_MAX_DUTY ≈ 50 %`. One bug or one test sketch without it = 2× overspeed | Hardware. Full duty is safe |
| Motor stress | 24 V pulses on brushes and commutator: more arcing, heat and noise for the same speed | Within rating |
| Control | Duty→speed is repeatable but twice as steep; usable range is 0–512 of 1023 | Full 0–1023 range |
| Spin-up | Faster: higher torque at low duty | Normal |
| Parts | Nothing to buy | Buck 36/48 V → 12 V, 30 A, same IP68 family. Everything else stays |

Until a 12 V buck is fitted, the firmware carries a hard constant

```
WHEEL_MAX_DUTY = 50 %      // 12 V average on a 24 V rail; becomes 100 % on a 12 V rail
```

that the UI cannot exceed. The UI's "100 % speed" maps to the cap.

Current ripple is the other thing to know here. It grows with rail voltage and shrinks with
PWM frequency: at the Uno's ~1 kHz on a low-inductance 775, the current ripple on 24 V is
enormous (heat, brush arcing, noise); at 16 kHz it is a few amps on either rail. So whichever
rail is used, run the modules near their 20 kHz limit.

> ❓ **OPEN:** Tilen decides between the two columns (`PARTS.md` #7). Also needed: the highest
> `analogWrite` value the Uno prototype used for a normal shot (0–255). Above 128 means those
> shots already used more than 12 V average, and a 12 V rail would shorten the maximum range.

## Direction

The MOSFET module switches one direction only. The two disks counter-rotate, so the leads of
one motor are swapped. Mark the wheels L and R and the motor leads + / − once and for all.

## ESP32 side

- PWM comes from the LEDC peripheral, 10-bit resolution (0–1023). The module accepts
  0–20 kHz. Target **16 kHz** (`WHEEL_PWM_HZ`), which keeps current ripple small on either
  rail. If the module's heatsink runs hot at working duty, drop to 10 kHz. The Uno's 490/980 Hz
  is fine only for a first bring-up.
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

- Motor and rail wires: ≥ 1.5 mm² (2.5 mm² on a 12 V rail), twisted pair per motor, short,
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
- A proper H-bridge / ESC if braking or reversing is ever wanted.
