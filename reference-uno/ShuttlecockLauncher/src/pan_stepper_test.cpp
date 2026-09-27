#include <Arduino.h>
#include <AccelStepper.h>

// ============================================================
// Pan stepper + two-button direction test
//
// Uses the SAME pins as src/main.cpp:
// - Pan stepper: STEP=2, DIR=3, MICROSTEP=9
// - Buttons: CONFIRM=6, MOVE=7 (INPUT_PULLUP => pressed == LOW)
//
// NOTE:
// PlatformIO/Arduino typically expects only one compilation unit
// defining setup()/loop(). To run this test, either:
// - temporarily rename src/main.cpp to something else, OR
// - set build_src_filter in platformio.ini to compile only this file.
// ============================================================

// Pan stepper pins
//VVVVVVVVVVVVVVVVVVVVV for testing the pan stepper
//#define PAN_STEP_PIN 2
//#define PAN_DIR_PIN 3
//VVVVVVVVVVVVVVVVVVVVV for testing the tilt stepper
#define PAN_STEP_PIN 0
#define PAN_DIR_PIN 1
#define PAN_MICROSTEP_PIN 9

// Button pins (same as main.cpp)
#define CONFIRM_BUTTON_PIN 6
#define MOVE_BUTTON_PIN 7

const float STEP_SPEED = 1200.0f;

// Phase lock: when starting from rest, run at low speed briefly so the rotor locks to the
// correct phase before full speed. Stops "same button sometimes CW, sometimes CCW" issue.
const float PHASE_LOCK_SPEED = 150.0f;   // low speed (steps/sec) during phase lock
const unsigned long PHASE_LOCK_MS = 60;   // how long to run at low speed before ramping up

// Smooth deceleration on button release.
const float DECEL_DECAY = 0.95f;
const float SPEED_STOP_THRESHOLD = 20.0f;

// AccelStepper in DRIVER mode: STEP, DIR
AccelStepper panStepper(AccelStepper::DRIVER, PAN_STEP_PIN, PAN_DIR_PIN);

static float currentSpeed = 0.0f;
static bool phaseLockActive = false;
static unsigned long phaseLockStart = 0;
static int phaseLockDirection = 1;  // +1 or -1

static inline bool isButtonPressed(uint8_t pin) {
  return digitalRead(pin) == LOW; // INPUT_PULLUP: LOW when pressed
}

void setup() {
  // Stepper pins
  pinMode(PAN_STEP_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN, OUTPUT);
  digitalWrite(PAN_STEP_PIN, LOW);
  digitalWrite(PAN_DIR_PIN, LOW);

  // DRV8825 microstepping configuration:
  // M0 and M1 pins are both connected to pin 9
  // Pin 9 = HIGH → M0=HIGH, M1=HIGH → 1/8 step mode (8x microstepping)
  // Pin 9 = LOW  → M0=LOW,  M1=LOW  → Full step mode (1x)
  pinMode(PAN_MICROSTEP_PIN, OUTPUT);
  //digitalWrite(PAN_MICROSTEP_PIN, HIGH);  // 1/8 step mode (8x microstepping)
  digitalWrite(PAN_MICROSTEP_PIN, LOW);   // Full step mode (1x)

  // Buttons (same electrical convention as main.cpp)
  pinMode(CONFIRM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOVE_BUTTON_PIN, INPUT_PULLUP);

  // Speed settings: 1/8 step, 500 microsteps/sec ≈ 18.75 RPM (smooth, was working)
  panStepper.setMaxSpeed(STEP_SPEED);
  panStepper.setAcceleration(500.0f);   // steps/sec^2 (not used by runSpeed, but harmless)
  panStepper.setSpeed(0.0f);
}

void loop() {
  const bool dirA = isButtonPressed(CONFIRM_BUTTON_PIN);
  const bool dirB = isButtonPressed(MOVE_BUTTON_PIN);

  if (phaseLockActive && (millis() - phaseLockStart >= PHASE_LOCK_MS)) {
    phaseLockActive = false;
    currentSpeed = (float)(phaseLockDirection) * STEP_SPEED;
  }

  if (dirA && !dirB) {
    if (currentSpeed == 0.0f) {
      // Starting from rest: phase lock so rotor syncs before full speed
      phaseLockActive = true;
      phaseLockStart = millis();
      phaseLockDirection = 1;
      currentSpeed = PHASE_LOCK_SPEED;
    } else {
      currentSpeed = STEP_SPEED;
    }
  } else if (dirB && !dirA) {
    if (currentSpeed == 0.0f) {
      phaseLockActive = true;
      phaseLockStart = millis();
      phaseLockDirection = -1;
      currentSpeed = -PHASE_LOCK_SPEED;
    } else {
      currentSpeed = -STEP_SPEED;
    }
  } else {
    // Buttons released: ramp down
    if (currentSpeed != 0.0f) {
      currentSpeed *= DECEL_DECAY;
      if (currentSpeed > -SPEED_STOP_THRESHOLD && currentSpeed < SPEED_STOP_THRESHOLD) {
        currentSpeed = 0.0f;
      }
    }
  }

  panStepper.setSpeed(currentSpeed);
  panStepper.runSpeed();
}

