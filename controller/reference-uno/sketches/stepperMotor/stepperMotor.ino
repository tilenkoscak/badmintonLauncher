#include <AccelStepper.h>

// Define the stepper motor connections
// #define STEP_PIN 2
// #define DIR_PIN 3
#define STEP_PIN 0
#define DIR_PIN 1

// Create an instance of the AccelStepper class
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  // Set the maximum speed and acceleration for the stepper motor
  stepper.setMaxSpeed(3000); // Steps per second
  stepper.setAcceleration(3000); // Steps per second^2

}

void loop() {
  // Rotate 360 degrees (200 steps for a 1.8 degree stepper motor)
  stepper.moveTo(125);
  stepper.runToPosition();

  delay(1000); // Wait for a second

  // Rotate 360 degrees in the opposite direction
  stepper.moveTo(-125);
  stepper.runToPosition();

  delay(1000); // Wait for a second
}
