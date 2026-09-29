# Reference: Arduino Uno prototype code

Everything Tilen wrote for the Uno-based prototype, kept verbatim. It is the source of truth
for servo angles, stepper speeds, the feed sequence and the pin habits that are known to work
on the real mechanism. Nothing here is compiled or shipped with the ESP32 firmware; the port
lives in `../firmware/`. This file summarises what the code says so nobody has to re-read it.

Two code bases exist. Both were written to test parts **individually**; there was never an
integrated "whole robot" program, which is what `../firmware/` will be.

| Folder | Toolchain | Content |
|---|---|---|
| `ShuttlecockLauncher/` | PlatformIO (VS Code / Cursor), board `uno`, AccelStepper 1.64 + Servo | The later, more complete code. `src/main.cpp` drives all three steppers and both servos: feeder homing, the reload sequence, a button-driven limit teach-in for pan and tilt. Three single-purpose tests next to it, selected with `build_src_filter` in `platformio.ini` |
| `sketches/` | Arduino IDE | Earlier stand-alone sketches, one folder each: stepper jog tests, servo tests, the feeder reload sequence, the DC-motor soft-start test. `sketches/libraries/AccelStepper/` is the stock library v1.64, vendored so the sketches compile; install it from Library Manager instead |

## Which file answers what

| Question | Look at |
|---|---|
| Feed sequence, servo angles, delays | `ShuttlecockLauncher/src/main.cpp` → `reload()` and the `open…/close…Servo()` helpers. Same code in `sketches/badmintonFeederRealoadSequence/` |
| Feeder homing against the optical endstop | `main.cpp` → `setFeedingZero()` |
| Pan/tilt speeds and the manual limit teach-in | `main.cpp` → `setupSteppers()`, `limitSetup()`, `moveToLimits()` |
| Launch-wheel PWM test (kick start + ramp) | `sketches/launcher_motors_control/launcher_motors_control.ino` |
| Stepper jog with two buttons | `ShuttlecockLauncher/src/pan_stepper_test.cpp` |
| Finding servo angles with two buttons | `ShuttlecockLauncher/src/servo_test.cpp` |
| Checking Uno pins | `ShuttlecockLauncher/src/io_pin_scan_test.cpp` |
| Early experiments | `sketches/stepperMotor`, `smallerStepper`, `button_controlled_stepper`, `servoWithPotentiometer`, `Sweep_servo` (the Arduino example), `burner` (servo burn-in, 15 s open/close cycles) |

## Uno pin usage (`main.cpp`)

| Uno pin | Use | Note |
|---|---|---|
| 0, 1 | Tilt STEP, DIR | The UART pins. Serial was moved to SoftwareSerial on A0/A1 to free them. Not needed on the ESP32 |
| 2, 3 | Pan STEP, DIR | |
| 12, 13 | Feeder STEP, DIR | |
| 9 | M0+M1 of the drivers, driven HIGH | One GPIO sets microstepping for all three drivers (see the inconsistency below) |
| 4 | Feeder limiter (optical endstop SIGNAL) | `pinMode(INPUT)`, no pull-up; **LOW = triggered**. The board drives the line itself |
| 6, 7 | CONFIRM and MOVE buttons | `INPUT_PULLUP`, pressed = LOW |
| 10, 11 | Arms servo, spoon servo | A Cursor rule in the project says this Uno's D10/D11 are faulty as inputs; the servo test therefore moved to D9 |
| 5 | Launch-wheel PWM (test sketch) | `analogWrite`, Timer0 ⇒ **980 Hz** |

## Facts the code establishes

### Feeder (servos + stepper + optical endstop)

Servo positions and settle delays, in Uno `Servo` degrees (pulse range 544–2400 µs):

| Servo | Closed | Open | Other | Settle delays used |
|---|---|---|---|---|
| Arms (`armsServo`) | **10°** | **40°** | – | open 550 ms, close 300 ms |
| Spoon (`spoonServo`) | **180°** | **0°** | "guiding position" **105°** | open 600 ms, close 750 ms, guiding 600 ms |

Feeder stepper: DRV8825 at **1/8 microstepping** (M0 and M1 tied together and driven HIGH),
1600 microsteps per revolution. One feed stroke is **2.5 revolutions = 4000 microsteps**.
Commanded 500 RPM and 3000 RPM/s, but AccelStepper on a 16 MHz Uno tops out around
4000 steps/s, so the real speed was roughly 150 RPM. Do not copy 500 RPM into the ESP32
firmware as a "known good" value; start near 150 RPM and tune upward.

Homing (`setFeedingZero`): run **toward** the endstop at 1200 steps/s until it triggers, back
off at 200 steps/s until it releases, set position 0 there. Ready position is then
2.5 revolutions **back** from the endstop.

The reload sequence, one shuttle (`reload()`):

1. Arms open (40°), 550 ms
2. Arms close (10°), 300 ms
3. Spoon to guiding position (105°), 600 ms
4. Spoon open (0°), 600 ms
5. Feeder stepper 2.5 revolutions forward
6. Wait 2000 ms
7. Feeder stepper 2.5 revolutions back
8. Spoon close (180°), 750 ms

About 5 s of servo time plus two feeder strokes, so roughly 7–8 s per shuttle as written.

Two safety habits the code insists on, both to be kept in the firmware:

- **Move the feeder back before any servo moves** ("as a safety precaution").
- **Write the closed position immediately after `attach()`**: an attached servo drifts to 90°
  otherwise, and at 90° the arms hit an obstacle and the spoon blocks the feeder.

In `loop()` the limiter is also checked every pass; if it reads triggered outside homing the
code calls `emergencyStop()` (stop all steppers, disable outputs, halt).

### Pan and tilt

- Speed 50 RPM, acceleration 50 RPM/s, software microstep multiplier 2 ⇒ 333 steps/s.
  Gentle. (Comments say "quarter step" while the constant is 2 and pin 9 wiring would give
  1/8: the physical jumpering of the pan/tilt drivers is not certain from the code.)
- **No endstops.** `limitSetup()` teaches the limits at every power-up: hold MOVE to drive
  tilt down, CONFIRM sets that as position 0; drive up, CONFIRM sets the upper limit; same
  for pan left (0) and right. `moveTo(0)` afterwards therefore goes to the lower/left limit,
  not the centre as the comment claims.
- `pan_stepper_test.cpp` adds a "phase lock" (60 ms at 150 steps/s before full speed) to cure
  "same button sometimes CW, sometimes CCW". That symptom points at DIR changing too close to
  a STEP edge or at the pins 0/1 hack, not at the motor. Not needed on the ESP32.

### Launch wheels

`launcher_motors_control.ino`, one motor on pin 5: guaranteed OFF at start, **kick at duty
255 for 200 ms**, drop to duty 100 (39 %), ramp 100 → 255 → 100 in steps of 10 every 50 ms,
stop. So the prototype did run a 775 at full duty on the 24 V rail during this test. There is
**no shot-tuning code**; which duty gives a good shot is not recorded anywhere.

### Optical endstop

Read as `INPUT` without pull-up, LOW when triggered, HIGH otherwise, powered from the Uno's
5 V. It is the feeder's home sensor and its safety limit. For the ESP32 it must be powered
from 3.3 V or its output divided (`../../wiring/06-optical-endstop.md`).

## Porting notes for the ESP32 firmware

- **Servo pulse range:** the Uno `Servo` library maps 0–180° onto 544–2400 µs. Use
  `attach(pin, 544, 2400)` in ESP32Servo so 10°, 40°, 105° and 180° land on the same physical
  positions. With 500–2500 µs the positions shift by a few degrees.
- **AccelStepper ports 1:1** (the library supports ESP32) and the ESP32 easily delivers the
  step rates the Uno could not. FastAccelStepper only if hardware-timed pulses are needed later.
- **Microstepping:** hard-wire M0–M2 with jumpers per `../../wiring/pin-map.md` instead of
  driving them from a GPIO; the multiplier then lives in `config.h` per axis.
- **Feeder speed:** start at ~150 RPM (what actually ran), tune up.
- **Pan/tilt limits:** offer the same teach-in from the web app and persist the limits in
  flash. Endstops for pan and tilt become optional.
- **Keep the two safety habits** (feeder back first, closed position on attach) and the
  limiter check as the feeder interlock.
- **Wheels:** the kick start is worth keeping as an option, but on a 12 V rail a plain ramp is
  likely enough. PWM frequency moves from 980 Hz to ~16 kHz.
