#include <AccelStepper.h>
#include <Servo.h>

// Define stepper pins
#define STEP_PIN 2      // Step pin
#define DIR_PIN 3       // Direction pin

// Define button pins
#define BUTTON_CW_PIN 4  // Button for clockwise rotation (FORWARD)
#define BUTTON_CCW_PIN 5 // Button for counterclockwise rotation (BACKWARD)

// Define microstepping control pins
#define M0_M1_PIN 8     // Shared pin for M0 and M1 (both HIGH for 1/8 microstepping)

// Define limiter pin
#define limiterPin 4

Servo armsServo;  // create servo object to control a servo
Servo spoonServo;
// twelve servo objects can be created on most boards

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 8;
// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
// Declare Max_Speed_StepsPerSec as a global variable
float Max_Speed_StepsPerSec;
float Accel_StepsPerSec2;

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
  Accel_StepsPerSec2 = microstepSetting * stepsPerRevolution * AccelRPMperSec / 60;
  stepper.setAcceleration(Accel_StepsPerSec2);
  
  // Set button pins as input pullup
  pinMode(BUTTON_CW_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CCW_PIN, INPUT_PULLUP);
  // Set limiter button pin as input pullup as well
  pinMode(limiterPin, INPUT);

  //move the feeder back before doing any movement on the spoonServo as a safety precaution
  moveFeeder("back", 2.5);

  armsServo.attach(10);  // attaches the armsServo on pin 10 to the armsServo object
  closeArmsServo();  // we have to immediatelly tell it to close otherwise it will try to go to position 90 and will run into an obstacle
  spoonServo.attach(11);  // attaches the spoonServo on pin 11 to the spoonServo object
  closeSpoonServo();  // we have to immediatelly tell it to close otherwise it will try to go to position 90 and will get in the way of the feeder

  // prepare the starting position for the reload sequance()
  setZero();
  moveFeeder("back", 2.5);
  
}


void moveSpoonServoTo(String position) {
  if (position == "open") {
    openSpoonServo();
  } else if (position == "close" || position == "closed") {
    closeSpoonServo();
  } else if (position == "guiding" || position == "guiding position") {
    spoonServo.write(105);
    delay(600);
  }
  
}
void openSpoonServo() {
  spoonServo.write(0);
  delay(600);
}
void closeSpoonServo() {
  spoonServo.write(180);
  delay(750);
}
void openArmsServo() {
  armsServo.write(40);
  delay(550);
}
void closeArmsServo() {
  armsServo.write(10);
  delay(300);
}



bool limitReached() {
  if (digitalRead(limiterPin) == LOW) {  // PRESSED
    return true;
  } else {
    return false;
  }
}

void setZero() {
  Serial.println("Homing...");
  // stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper.setMaxSpeed(1200);
  stepper.setAcceleration(300);

  // Move in negative direction until limiter triggered
  stepper.setSpeed(1200);
  // stepper.setSpeed(Max_Speed_StepsPerSec);
  while (!limitReached()) {
    stepper.runSpeed();
  }
  //once the limit is reached move sleightly back so the limit switch isn't being triggered anymore
  stepper.setSpeed(-200);
  while (limitReached()) {
    stepper.runSpeed();
  }

  // Stop, set current position as zero
  stepper.stop();
  stepper.setCurrentPosition(0);
  Serial.println("Zero position set.");
}

void emergencyStop() {
  Serial.println("Emergency stop triggered!");
  stepper.stop();              // Stop smoothly
  while (stepper.isRunning()) {
    stepper.run();
  }
  stepper.disableOutputs();    // Cut off power to motor driver

  Serial.println("Motor stopped. Code halted.");
  while (1); // Halt program
}

// Function to calculate steps based on desired rotations
float convert_rotational_position_to_steps(float rotations) {
  return rotations * stepsPerRevolution * microstepSetting;
}

  /*
  Moves the feeding mechanism back and forth
  @param direction(String): "forward"/"forwards" or "back"/"backward"/"backwards" 
  @param rotationts(float): the amount of rotations the feeding mechanism stepper will do
  */
void moveFeeder(String direction , float rotations) {
  // long steps = microstepSetting * stepsPerRevolution * 2;
  long steps = convert_rotational_position_to_steps(rotations);

  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper.setSpeed(Max_Speed_StepsPerSec);
  stepper.setAcceleration(Accel_StepsPerSec2);

  if (direction == "forward" || direction == "forwards") {
    stepper.move(steps);
  } else if (direction == "back" || direction == "backward" || direction == "backwards") {
    stepper.move(-steps);
  }

  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
}

// Reload function
void reload() {
  float move_by_this_many_rotations = 2.5;

  openArmsServo();
  closeArmsServo();
  moveSpoonServoTo("guiding position");
  openSpoonServo();
  moveFeeder("forward", move_by_this_many_rotations);
  delay(2000);
  moveFeeder("back", move_by_this_many_rotations);
  closeSpoonServo();
  // delay(1000);
}


void loop() {
  if (limitReached()) {
    emergencyStop();
  }
  // // Read button states
  // bool buttonCWState = digitalRead(BUTTON_CW_PIN);
  // bool buttonCCWState = digitalRead(BUTTON_CCW_PIN);
  // Serial.println(buttonCWState);
  // Serial.println(buttonCCWState);
  // // Control stepper motor based on button states
  // if (buttonCWState == HIGH) {
  //   // Move clockwise
  //   stepper.setSpeed(Max_Speed_StepsPerSec); // Set positive speed for CW
  //   stepper.runSpeed();
  // } else if (buttonCCWState == HIGH) {
  //   // Move counterclockwise
  //   stepper.setSpeed(-Max_Speed_StepsPerSec); // Set negative speed for CCW
  //   stepper.runSpeed();
  // } else {
  //   // Stop the motor if no button is pressed
  //   stepper.setSpeed(0);
  //   stepper.runSpeed();
  // }
  reload();  
}