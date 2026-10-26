# 05 · Power distribution

Draft based on the salvaged **Artillery Sidewinder X3 Plus** power supply, which has two
outputs: **24 V / 4.2 A** and **36 V / 9.7 A**. The robot is mains-powered.

The negatives of the two outputs are **not** connected inside the PSU (measured 2026-10-21),
so the star point below is what joins the two sides.

> ❓ **OPEN:** Before I mark this page final I need a photo of the PSU label (model and
> ratings) and whether it has a mains switch and a fan (`PARTS.md` #6).

## Power tree

```
 Mains 230 V ──► Artillery X3 Plus PSU
                  │
                  ├── OUTPUT 1: 24 V / 4.2 A ──────────────────────────────────────── "aim & feed rail"
                  │      ├──► DRV8825 pan   VMOT  (100 µF/50 V on the driver)
                  │      ├──► DRV8825 tilt  VMOT
                  │      ├──► DRV8825 feed  VMOT
                  │      ├──► MEAN WELL N7805-1CW ──► 5 V / 1 A ──► ESP32 `5Vin` pin      (recommended use)
                  │      └──► 5–6 V buck ≥ 3 A (to add) ──(+1000 µF)──► arms servo, lift servo
                  │
                  └── OUTPUT 2: 36 V / 9.7 A ──► buck 12 V / 30 A (ordered; 24 V one until then) ──► [ E-STOP / WHEEL SWITCH ]
                                                                               │
                                                        ┌──────────────────────┴──────────────────────┐
                                                        │ fuse 15 A                                   │ fuse 15 A
                                                        ▼                                             ▼
                                                  1000 µF + 1.5KE30A across DC+/DC−             1000 µF + 1.5KE30A
                                                  MOSFET module L  ◄─ PWM GPIO 12             MOSFET module R  ◄─ PWM GPIO 13
                                                        ▼                                             ▼
                                                  775 motor L, E83-004 flyback across it        775 motor R, E83-004 flyback across it

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
| Wheel rail | **12 V** (24 V until the new buck is fitted) | PSU output 2 via buck | 2 × MOSFET module → 775 motors | A few amps per wheel at steady speed; tens of amps per wheel for a moment at spin-up, kept in check by the firmware ramp | **≈ 26 A at 12 V** (350 W × ~90 %), less than the buck's 30 A |
| ESP32 | 5 V | N7805 | ESP32 board only | ≤ 0.6 A | 1 A |
| Servo rail | 5 V (6 V if the buck allows) | new buck | 2 × Miuzei 15 kg | ≈ 0.5 A idle, 2–3 A per servo when stalled | ≥ 3 A recommended |

The 12 V rail matches the motors. The firmware's duty cap only bites while the old 24 V buck
is still fitted, see `04-launch-wheels-dc-motors.md`.

## Rules

- **Power-up order:** ESP32 first (its 5 V comes straight from output 1, so it is up as soon
  as the PSU is), then the wheel switch. Power-down in reverse. The pull-up on stepper
  ENABLE and the pull-downs on the wheel PWM lines (`pin-map.md`) cover the case where this
  order is not respected.
- **E-STOP / wheel switch** sits between the buck and the two wheel branches and must be
  reachable without a hand near the disks. It cuts only the wheel rail; the ESP32 stays alive
  so the phone keeps its connection and the app can show "wheel power off". Steppers and
  servos are handled by firmware: pan and tilt stop, the feeder parks
  (`controller/README.md`). Optional later: a relay on the 24 V aim & feed rail driven
  by the ESP32.
- **Never route motor current through the ESP32 board** or through a breadboard.
- **Common ground, once, and it is essential.** Both PSU outputs, the buck and the ESP32 meet
  at the star point. The two outputs are isolated from each other inside the PSU (measured),
  so this point is the only link between the 24 V side and the wheel side: without it the
  ESP32's wheel PWM has no reference at the MOSFET modules and the wheels can misbehave. Each
  output's - gets its own wire to the star, as thick as that output's + wire.
- **Flyback diodes:** each E83-004 sits across its motor and carries the motor current only
  during the PWM off-time, 1–2 W. A small heatsink is plenty; the tab is the cathode, so use
  insulating washers if it shares a heatsink.
- **Buffer capacitors and clamps:** 100 µF at every DRV8825 (fitted), 1000 µF plus the
  1.5KE30A TVS across each MOSFET module's input DC+/DC− (fitted), 470–1000 µF on the servo
  rail (to add).
- **Soft start** for the 775 motors in firmware, and start the second wheel a moment after
  the first, so the buck and PSU output 2 do not see two stall currents at the same instant.
- **Fuses:** 15 A automotive blade per wheel branch; optional 4 A in the stepper feed.
  Confirm after measuring real currents.
- **Wire gauge:** 2.5 mm² for the wheel branches and the buck leads, 0.5 mm² for steppers
  and servos, anything for signals.
- **Brown-out:** if the ESP32 resets when motors start, its 5 V is sagging. That is the
  reason it gets the N7805 for itself.
- **The buck is non-isolated**: its input − and output − are the same node. Fine, because
  everything shares the star ground anyway.
