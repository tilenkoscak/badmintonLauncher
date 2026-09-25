# 05 · Power distribution

Draft based on the salvaged **Artillery Sidewinder X3 Plus** power supply, which has two
outputs: **24 V / 4.2 A** and **36 V / 9.7 A**. The robot is mains-powered.

> ❓ **OPEN:** Before I mark this page final I need (see `PARTS.md` #6, #7): the PSU model
> label, whether the negatives of the two outputs are already connected inside the PSU, the
> buck converter's current variant and measured output voltage, and confirmation of the
> N7805 marking.

## Power tree

```
 Mains 230 V ──► Artillery X3 Plus PSU
                  │
                  ├── OUTPUT 1: 24 V / 4.2 A ──────────────────────────────────────── "aim & feed rail"
                  │      ├──► DRV8825 pan   VMOT  (100 µF/50 V on the driver)
                  │      ├──► DRV8825 tilt  VMOT
                  │      ├──► DRV8825 feed  VMOT
                  │      ├──► MEAN WELL N7805-1CW ──► 5 V / 1 A ──► ESP32 `5V` pin        (recommended use)
                  │      └──► 5–6 V buck ≥ 3 A (to add) ──(+1000 µF)──► servo A, servo B
                  │
                  └── OUTPUT 2: 36 V / 9.7 A ──► buck 36 → 24 V (IP68) ──► [ E-STOP / WHEEL SWITCH ]
                                                                               │
                                                        ┌──────────────────────┴──────────────────────┐
                                                        │ fuse 10 A                                   │ fuse 10 A
                                                        ▼                                             ▼
                                                  E83-004 (series)                              E83-004 (series)
                                                        ▼                                             ▼
                                                  MOSFET module L  ◄─ PWM GPIO 12             MOSFET module R  ◄─ PWM GPIO 13
                                                        ▼                                             ▼
                                                  775 motor L (+TVS, +flyback)                  775 motor R (+TVS, +flyback)

 GROUND: one star point next to the PSU. Output 1 −, output 2 −, buck −, both MOSFET module DC−,
 the DRV8825 motor grounds and the ESP32 GND each get their own wire to that point.
```

What the prototype already does: output 1 feeds the steppers and the N7805 → servos;
output 2 feeds the buck → wheels. What changes for the ESP32 build: the ESP32 needs its own
5 V (proposal: the N7805), the servos get a stronger 5–6 V buck, and the wheel rail gets a
switch and fuses.

## Rails and budget

| Rail | Voltage | Source | Feeds | Estimated draw | Available |
|---|---|---|---|---|---|
| Aim & feed rail | 24 V | PSU output 1 | 3 × DRV8825, N7805, servo buck | Steppers ≈ 1.5–3 A from the supply (the chopper drivers draw less than the phase current), servo buck ≤ 1 A, N7805 ≈ 0.15 A ⇒ **≈ 3–4 A** | 4.2 A. Tight if all three steppers are set to 1.7 A; set the feeder lower |
| Wheel rail | 24 V | PSU output 2 via buck | 2 × MOSFET module → 775 motors | 2 × ≈ 6 A average at full 150 W and 50 % duty; much more for a fraction of a second at spin-up | **≈ 13 A total** (350 W × ~90 % / 24 V), whatever the buck variant says |
| ESP32 | 5 V | N7805 | ESP32 board only | ≤ 0.6 A | 1 A |
| Servo rail | 5 V (6 V if the buck allows) | new buck | 2 × Miuzei 15 kg | ≈ 0.5 A idle, 2–3 A per servo when stalled | ≥ 3 A recommended |

The 24 V wheel rail with 12 V motors is the reason for the firmware's `WHEEL_MAX_DUTY` cap
(≈ 50 %), see `04-launch-wheels-dc-motors.md`.

## Rules

- **Power-up order:** ESP32 first (its 5 V comes straight from output 1, so it is up as soon
  as the PSU is), then the wheel switch. Power-down in reverse. The pull-up on stepper
  ENABLE and the pull-downs on the wheel PWM lines (`pin-map.md`) cover the case where this
  order is not respected.
- **E-STOP / wheel switch** sits between the buck and the two wheel branches and must be
  reachable without a hand near the disks. It cuts only the wheel rail; the ESP32 stays alive
  so the phone keeps its connection and the app can show "wheel power off". Steppers and
  servos are stopped by firmware. Optional later: a relay on the 24 V aim & feed rail driven
  by the ESP32.
- **Never route motor current through the ESP32 board** or through a breadboard.
- **Common ground, once.** Both PSU outputs, the buck and the ESP32 meet at the star point.
  If the multimeter shows the two PSU negatives already connected, still run separate wires to
  the star; just do not be surprised by the continuity.
- **Series diode heat:** each E83-004 dissipates 3–4 W at full wheel power. Bolt both to a
  heatsink with insulating washers (the tab is the cathode).
- **Buffer capacitors:** 100 µF at every DRV8825 (fitted), 1000 µF across each MOSFET
  module's **input** DC+/DC− (move them there if they are across the motor today, see
  `PARTS.md` #10), 470–1000 µF on the servo rail.
- **Soft start** for the 775 motors in firmware, and start the second wheel a moment after
  the first, so the buck and PSU output 2 do not see two stall currents at the same instant.
- **Fuses:** 10 A automotive blade per wheel branch as a starting point; optional 4 A in the
  stepper feed. Confirm after measuring real currents.
- **Wire gauge:** ≥ 1.5 mm² for the wheel branches and the buck leads, 0.5 mm² for
  steppers and servos, anything for signals.
- **Brown-out:** if the ESP32 resets when motors start, its 5 V is sagging. That is the
  reason it gets the N7805 for itself.
- **The buck is non-isolated**: its input − and output − are the same node. Fine, because
  everything shares the star ground anyway.
