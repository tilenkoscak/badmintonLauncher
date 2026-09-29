#include <AccelStepper.h>

// Define stepper pins
#define STEP_PIN 2      // Step pin
#define DIR_PIN 3       // Direction pin

// Define button pins
#define BUTTON_CW_PIN 4  // Button for clockwise rotation
#define BUTTON_CCW_PIN 5 // Button for counterclockwise rotation

// Define microstepping control pins
#define M0_M1_PIN 8     // Shared pin for M0 and M1 (both HIGH for 1/8 microstepping)

// Define limiter pin
#define limiterPin 12

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 8;

// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Declare Max_Speed_StepsPerSec as a global variable
float Max_Speed_StepsPerSec;

// Function to calculate steps based on desired rotations
float convert_rotational_position_to_steps(float rotations) {
  return rotations * stepsPerRevolution * microstepSetting;
}

void setup() {
  // Set microstepping pins as output
  pinMode(M0_M1_PIN, OUTPUT);

  // Set microstepping mode to 1/32 (M0=HIGH, M1=LOW, M2=HIGH)
  digitalWrite(M0_M1_PIN, HIGH); // M0 and M2 are both HIGH

  // Set the max speed and acceleration
  float MaxRPM = 500; // Set max speed in rpm (revolutions per minute)
  Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  
  float AccelRPMperSec = 3000; // Set acceleration in rpm/sec
  float Accel_StepsPerSec2 = microstepSetting * stepsPerRevolution * AccelRPMperSec / 60;
  stepper.setAcceleration(Accel_StepsPerSec2);
  
  // Set button pins as input pullup
  pinMode(BUTTON_CW_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CCW_PIN, INPUT_PULLUP);
  // Set limiter button pin as input pullup as well
  pinMode(limiterPin, INPUT);
}

bool limitReached() {
  if (digitalRead(limiterPin) == LOW) {  // PRESSED
    return true;
  } else {
    return false;
  }
}

void setZero() {

}

void MoveBack(int) {
  
}

void loop() {
  setZero();
  if (!limitReached()) {
    // Read button states
    bool buttonCWState = digitalRead(BUTTON_CW_PIN);
    bool buttonCCWState = digitalRead(BUTTON_CCW_PIN);
    Serial.println(buttonCWState);
    Serial.println(buttonCCWState);
    // Control stepper motor based on button states
    if (buttonCWState == HIGH) {
      // Move clockwise
      stepper.setSpeed(Max_Speed_StepsPerSec); // Set positive speed for CW
      stepper.runSpeed();
    } else if (buttonCCWState == HIGH) {
      // Move counterclockwise
      stepper.setSpeed(-Max_Speed_StepsPerSec); // Set negative speed for CCW
      stepper.runSpeed();
    } else {
      // Stop the motor if no button is pressed
      stepper.setSpeed(0);
      stepper.runSpeed();
    }
  } else {
    while(1);
  }
  
}