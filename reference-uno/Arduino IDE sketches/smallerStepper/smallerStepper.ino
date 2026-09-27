#include <AccelStepper.h>

// Define stepper pins
#define STEP_PIN 2      // Step pin
#define DIR_PIN 3       // Direction pin


// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 1;

// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Function to calculate steps based on desired rotations
float convert_rotational_position_to_steps(float rotations) {
  return rotations * stepsPerRevolution * microstepSetting;
}

void setup() {

  // Set the max speed and acceleration
  //float MaxRPM = 300; // Set max speed in rpm (revolutions per minute)
  float MaxRPM = 60; // Set max speed in rpm (revolutions per minute)
  float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  
  //float AccelRPMperSec = 3000; // Set acceleration in rpm/sec
  float AccelRPMperSec = 600;
  float Accel_StepsPerSec2 = microstepSetting * stepsPerRevolution * AccelRPMperSec / 60;
  stepper.setAcceleration(Accel_StepsPerSec2);
  
  // Move to position 0 as the starting point
  //stepper.setCurrentPosition(0);
}

void loop() {
  // Move to 3 o'clock position
  stepper.moveTo(convert_rotational_position_to_steps(0.25));
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  delay(500);

  // Move to 9 o'clock position
  stepper.moveTo(convert_rotational_position_to_steps(0.75));
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  delay(500);

  // Move to 6 o'clock position
  stepper.moveTo(convert_rotational_position_to_steps(0.50));
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  delay(500);

  // Move to 12 o'clock position
  stepper.moveTo(convert_rotational_position_to_steps(1));
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  delay(500);

  // Move to starting position (will rotate in opposite direction)
  stepper.moveTo(convert_rotational_position_to_steps(0));
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  delay(2000);
}