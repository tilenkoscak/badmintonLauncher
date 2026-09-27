#include <Arduino.h>
#include <AccelStepper.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// SoftwareSerial for Serial communication (frees up pins 0 and 1 for stepper control)
// Using analog pins A0 (RX) and A1 (TX) which are digital pins 14 and 15
#define SOFT_SERIAL_RX_PIN A0  // Pin 14
#define SOFT_SERIAL_TX_PIN A1  // Pin 15
SoftwareSerial softSerial(SOFT_SERIAL_RX_PIN, SOFT_SERIAL_TX_PIN);

// Use SoftwareSerial for all Serial communication
// This allows pins 0 and 1 to be used for stepper control
#define Serial softSerial

// ==================== PIN DEFINITIONS ====================
// Feeding stepper pins
#define FEEDING_STEP_PIN 12
#define FEEDING_DIR_PIN 13
#define FEEDING_MICROSTEP_PIN 9

// Tilt stepper pins (controls machine tilt)
#define TILT_STEP_PIN 0
#define TILT_DIR_PIN 1
//#define TILT_STEP_PIN 2
//#define TILT_DIR_PIN 3
#define TILT_MICROSTEP_PIN 9

// Pan stepper pins (controls machine pan)
//#define PAN_STEP_PIN 0
//#define PAN_DIR_PIN 1
#define PAN_STEP_PIN 2
#define PAN_DIR_PIN 3
#define PAN_MICROSTEP_PIN 9

// Button pins
#define CONFIRM_BUTTON_PIN 6 // Confirm button for limit setup
#define MOVE_BUTTON_PIN 7    // Move button for limit setup

// Limiter pin for feeding stepper
#define FEEDING_LIMITER_PIN 4

// Servo pins
#define ARMS_SERVO_PIN 10
#define SPOON_SERVO_PIN 11

// ==================== SERVO OBJECTS ====================
Servo armsServo;
Servo spoonServo;

// ==================== STEPPER MOTOR CONSTANTS ====================
const float STEPS_PER_REVOLUTION = 200;

// Microstepping multipliers for each stepper
int feedingMicrostepSetting = 8;  // Keep smooth for feeding mechanism
int tiltMicrostepSetting = 2;     // Reduced to 4 (quarter step) for more torque
int panMicrostepSetting = 2;      // Reduced to 4 (quarter step) for more torque

// ==================== STEPPER MOTOR OBJECTS ====================
AccelStepper feedingStepper(AccelStepper::DRIVER, FEEDING_STEP_PIN, FEEDING_DIR_PIN);
AccelStepper tiltStepper(AccelStepper::DRIVER, TILT_STEP_PIN, TILT_DIR_PIN);
AccelStepper panStepper(AccelStepper::DRIVER, PAN_STEP_PIN, PAN_DIR_PIN);

// ==================== STEPPER MOTOR SPEED VARIABLES ====================
float feedingMaxSpeedStepsPerSec;
float feedingAccelStepsPerSec2;
float tiltMaxSpeedStepsPerSec;
float tiltAccelStepsPerSec2;
float panMaxSpeedStepsPerSec;
float panAccelStepsPerSec2;

// ==================== LIMIT POSITIONS ====================
long tiltLowerLimit = 0;  // Lower tilt limit position
long tiltUpperLimit = 0;  // Upper tilt limit position
long panLeftLimit = 0;    // Left pan limit position
long panRightLimit = 0;   // Right pan limit position

// ==================== FUNCTION DECLARATIONS ====================
void moveSpoonServoTo(String position);
void openSpoonServo();
void closeSpoonServo();
void openArmsServo();
void closeArmsServo();
bool feedingLimitReached();
void setFeedingZero();
void emergencyStop();
float convertRotationalPositionToSteps(float rotations, int microstepSetting);
void moveFeeder(String direction, float rotations);
void reload();
void setupSteppers();
void limitSetup();
void moveToLimits();
bool isButtonPressed(int pin);
void waitForButtonRelease(int pin);

void setup() {
  // Configure pins 0 and 1 as OUTPUT for tilt stepper BEFORE any Serial initialization
  // This ensures they're set up for stepper control and not Serial
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT);
  digitalWrite(TILT_STEP_PIN, LOW);
  digitalWrite(TILT_DIR_PIN, LOW);
  
  // Initialize SoftwareSerial (uses A0/A1, not pins 0/1)
  //Serial.begin(9600);
  
  // Set button pins first (needed for waiting)
  pinMode(CONFIRM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOVE_BUTTON_PIN, INPUT_PULLUP);
  
  // Wait for user to press CONFIRM button before starting
  //Serial.println("Press CONFIRM button to start setup...");
  while (!isButtonPressed(CONFIRM_BUTTON_PIN)) {
    delay(10);  // Small delay to avoid excessive CPU usage
  }
  waitForButtonRelease(CONFIRM_BUTTON_PIN);
  //Serial.println("Setup starting...");
  delay(500);
  
  // Setup all steppers
  setupSteppers();
  
  // Set limiter pin for feeding stepper
  pinMode(FEEDING_LIMITER_PIN, INPUT);
  
  // Move the feeder back before doing any movement on the spoonServo as a safety precaution
  moveFeeder("back", 2.5);
  
  // Initialize servos
  armsServo.attach(ARMS_SERVO_PIN);
  closeArmsServo();  // Immediately close to avoid hitting obstacles
  
  spoonServo.attach(SPOON_SERVO_PIN);
  closeSpoonServo();  // Immediately close to avoid interfering with feeder
  
  // Prepare the starting position for the reload sequence
  setFeedingZero();
  moveFeeder("back", 2.5);
  
  // Run limit setup
  limitSetup();
}

void setupSteppers() {
  // IMPORTANT: Explicitly configure all step and direction pins as OUTPUT
  // This is critical for pins 0 and 1 which were previously used for Serial
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT);
  pinMode(PAN_STEP_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN, OUTPUT);
  pinMode(FEEDING_STEP_PIN, OUTPUT);
  pinMode(FEEDING_DIR_PIN, OUTPUT);
  
  // Initialize pins to LOW
  digitalWrite(TILT_STEP_PIN, LOW);
  digitalWrite(TILT_DIR_PIN, LOW);
  digitalWrite(PAN_STEP_PIN, LOW);
  digitalWrite(PAN_DIR_PIN, LOW);
  digitalWrite(FEEDING_STEP_PIN, LOW);
  digitalWrite(FEEDING_DIR_PIN, LOW);
  
  // Setup feeding stepper microstepping
  pinMode(FEEDING_MICROSTEP_PIN, OUTPUT);
  digitalWrite(FEEDING_MICROSTEP_PIN, HIGH);
  
  // Setup tilt and pan stepper microstepping (shared pin)
  pinMode(TILT_MICROSTEP_PIN, OUTPUT);
  digitalWrite(TILT_MICROSTEP_PIN, HIGH);
  
  // Configure feeding stepper speed and acceleration
  float feedingMaxRPM = 500;
  feedingMaxSpeedStepsPerSec = feedingMicrostepSetting * STEPS_PER_REVOLUTION * feedingMaxRPM / 60;
  feedingStepper.setMaxSpeed(feedingMaxSpeedStepsPerSec);
  
  float feedingAccelRPMperSec = 3000;
  feedingAccelStepsPerSec2 = feedingMicrostepSetting * STEPS_PER_REVOLUTION * feedingAccelRPMperSec / 60;
  feedingStepper.setAcceleration(feedingAccelStepsPerSec2);
  
  // Configure tilt stepper speed and acceleration
  float tiltMaxRPM = 50;
  tiltMaxSpeedStepsPerSec = tiltMicrostepSetting * STEPS_PER_REVOLUTION * tiltMaxRPM / 60;
  tiltStepper.setMaxSpeed(tiltMaxSpeedStepsPerSec);
  
  //float tiltAccelRPMperSec = 3000;
  float tiltAccelRPMperSec = 50;
  tiltAccelStepsPerSec2 = tiltMicrostepSetting * STEPS_PER_REVOLUTION * tiltAccelRPMperSec / 60;
  tiltStepper.setAcceleration(tiltAccelStepsPerSec2);
  
  // Configure pan stepper speed and acceleration
  float panMaxRPM = 50;
  panMaxSpeedStepsPerSec = panMicrostepSetting * STEPS_PER_REVOLUTION * panMaxRPM / 60;
  panStepper.setMaxSpeed(panMaxSpeedStepsPerSec);
  
  //float panAccelRPMperSec = 3000;
  float panAccelRPMperSec = 50;
  panAccelStepsPerSec2 = panMicrostepSetting * STEPS_PER_REVOLUTION * panAccelRPMperSec / 60;
  panStepper.setAcceleration(panAccelStepsPerSec2);
}

void limitSetup() {
  Serial.println("=== Limit Setup Mode ===");
  Serial.println("Use MOVE button to position, then CONFIRM to set limit");
  
  // Set lower tilt limit
  Serial.println("\nSetting LOWER tilt limit...");
  Serial.println("Press MOVE button to move tilt DOWN, then CONFIRM when at desired position");
  while (!isButtonPressed(CONFIRM_BUTTON_PIN)) {
    if (isButtonPressed(MOVE_BUTTON_PIN)) {
      tiltStepper.setSpeed(-tiltMaxSpeedStepsPerSec);
      tiltStepper.runSpeed();
    } else {
      tiltStepper.setSpeed(0);
      tiltStepper.runSpeed();
    }
  }
  waitForButtonRelease(CONFIRM_BUTTON_PIN);
  tiltLowerLimit = tiltStepper.currentPosition();
  tiltStepper.stop();
  tiltStepper.setCurrentPosition(0);  // Set current position as reference
  Serial.print("Lower tilt limit set at position: ");
  Serial.println(tiltLowerLimit);
  delay(500);
  
  // Set upper tilt limit
  Serial.println("\nSetting UPPER tilt limit...");
  Serial.println("Press MOVE button to move tilt UP, then CONFIRM when at desired position");
  while (!isButtonPressed(CONFIRM_BUTTON_PIN)) {
    if (isButtonPressed(MOVE_BUTTON_PIN)) {
      tiltStepper.setSpeed(tiltMaxSpeedStepsPerSec);
      tiltStepper.runSpeed();
    } else {
      tiltStepper.setSpeed(0);
      tiltStepper.runSpeed();
    }
  }
  waitForButtonRelease(CONFIRM_BUTTON_PIN);
  tiltUpperLimit = tiltStepper.currentPosition();
  Serial.print("Upper tilt limit set at position: ");
  Serial.println(tiltUpperLimit);
  delay(500);
  
  // Set left pan limit
  Serial.println("\nSetting LEFT pan limit...");
  Serial.println("Press MOVE button to move pan LEFT, then CONFIRM when at desired position");
  while (!isButtonPressed(CONFIRM_BUTTON_PIN)) {
    if (isButtonPressed(MOVE_BUTTON_PIN)) {
      panStepper.setSpeed(-panMaxSpeedStepsPerSec);
      panStepper.runSpeed();
    } else {
      panStepper.setSpeed(0);
      panStepper.runSpeed();
    }
  }
  waitForButtonRelease(CONFIRM_BUTTON_PIN);
  panLeftLimit = panStepper.currentPosition();
  panStepper.stop();
  panStepper.setCurrentPosition(0);  // Set current position as reference
  Serial.print("Left pan limit set at position: ");
  Serial.println(panLeftLimit);
  delay(500);
  
  // Set right pan limit
  Serial.println("\nSetting RIGHT pan limit...");
  Serial.println("Press MOVE button to move pan RIGHT, then CONFIRM when at desired position");
  while (!isButtonPressed(CONFIRM_BUTTON_PIN)) {
    if (isButtonPressed(MOVE_BUTTON_PIN)) {
      panStepper.setSpeed(panMaxSpeedStepsPerSec);
      panStepper.runSpeed();
    } else {
      panStepper.setSpeed(0);
      panStepper.runSpeed();
    }
  }
  waitForButtonRelease(CONFIRM_BUTTON_PIN);
  panRightLimit = panStepper.currentPosition();
  Serial.print("Right pan limit set at position: ");
  Serial.println(panRightLimit);
  delay(500);
  
  // Print all limits
  Serial.println("\n=== All Limits Set ===");
  Serial.print("Tilt Lower Limit: ");
  Serial.println(tiltLowerLimit);
  Serial.print("Tilt Upper Limit: ");
  Serial.println(tiltUpperLimit);
  Serial.print("Pan Left Limit: ");
  Serial.println(panLeftLimit);
  Serial.print("Pan Right Limit: ");
  Serial.println(panRightLimit);
  Serial.println("=== Limit Setup Complete ===");
  
  // Return both steppers to center/zero position
  tiltStepper.moveTo(0);
  panStepper.moveTo(0);
  while (tiltStepper.distanceToGo() != 0 || panStepper.distanceToGo() != 0) {
    tiltStepper.run();
    panStepper.run();
  }
}

void moveToLimits() {
  // Move to upper tilt limit
  Serial.println("Moving to UPPER tilt limit...");
  tiltStepper.moveTo(tiltUpperLimit);
  while (tiltStepper.distanceToGo() != 0) {
    tiltStepper.run();
    panStepper.run();  // Keep pan stepper responsive
  }
  delay(1000);
  
  // Move to lower tilt limit (while moving pan to right)
  Serial.println("Moving to LOWER tilt limit and RIGHT pan limit simultaneously...");
  tiltStepper.moveTo(tiltLowerLimit);
  panStepper.moveTo(panRightLimit);
  while (tiltStepper.distanceToGo() != 0 || panStepper.distanceToGo() != 0) {
    tiltStepper.run();
    panStepper.run();
  }
  delay(1000);
  
  // Move to upper tilt limit (while moving pan to left)
  Serial.println("Moving to UPPER tilt limit and LEFT pan limit simultaneously...");
  tiltStepper.moveTo(tiltUpperLimit);
  panStepper.moveTo(panLeftLimit);
  while (tiltStepper.distanceToGo() != 0 || panStepper.distanceToGo() != 0) {
    tiltStepper.run();
    panStepper.run();
  }
  delay(1000);
  
  // Return to center
  Serial.println("Returning to center position...");
  tiltStepper.moveTo(0);
  panStepper.moveTo(0);
  while (tiltStepper.distanceToGo() != 0 || panStepper.distanceToGo() != 0) {
    tiltStepper.run();
    panStepper.run();
  }
  delay(1000);
}

bool isButtonPressed(int pin) {
  return digitalRead(pin) == LOW;  // PULLUP: LOW when pressed
}

void waitForButtonRelease(int pin) {
  while (isButtonPressed(pin)) {
    delay(10);
  }
  delay(50);  // Debounce
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

bool feedingLimitReached() {
  if (digitalRead(FEEDING_LIMITER_PIN) == LOW) {  // PRESSED
    return true;
  } else {
    return false;
  }
}

void setFeedingZero() {
  Serial.println("Homing feeding stepper...");
  feedingStepper.setMaxSpeed(1200);
  feedingStepper.setAcceleration(300);
  
  // Move in negative direction until limiter triggered
  feedingStepper.setSpeed(1200);
  while (!feedingLimitReached()) {
    feedingStepper.runSpeed();
  }
  
  // Once the limit is reached, move slightly back so the limit switch isn't being triggered anymore
  feedingStepper.setSpeed(-200);
  while (feedingLimitReached()) {
    feedingStepper.runSpeed();
  }
  
  // Stop, set current position as zero
  feedingStepper.stop();
  feedingStepper.setCurrentPosition(0);
  Serial.println("Feeding stepper zero position set.");
}

void emergencyStop() {
  Serial.println("Emergency stop triggered!");
  feedingStepper.stop();
  tiltStepper.stop();
  panStepper.stop();
  
  while (feedingStepper.isRunning() || tiltStepper.isRunning() || panStepper.isRunning()) {
    feedingStepper.run();
    tiltStepper.run();
    panStepper.run();
  }
  
  feedingStepper.disableOutputs();
  tiltStepper.disableOutputs();
  panStepper.disableOutputs();
  
  Serial.println("All motors stopped. Code halted.");
  while (1); // Halt program
}

// Function to calculate steps based on desired rotations
float convertRotationalPositionToSteps(float rotations, int microstepSetting) {
  return rotations * STEPS_PER_REVOLUTION * microstepSetting;
}

/*
  Moves the feeding mechanism back and forth
  @param direction(String): "forward"/"forwards" or "back"/"backward"/"backwards" 
  @param rotations(float): the amount of rotations the feeding mechanism stepper will do
*/
void moveFeeder(String direction, float rotations) {
  long steps = convertRotationalPositionToSteps(rotations, feedingMicrostepSetting);
  feedingStepper.setMaxSpeed(feedingMaxSpeedStepsPerSec);
  feedingStepper.setSpeed(feedingMaxSpeedStepsPerSec);
  feedingStepper.setAcceleration(feedingAccelStepsPerSec2);
  
  if (direction == "forward" || direction == "forwards") {
    feedingStepper.move(steps);
  } else if (direction == "back" || direction == "backward" || direction == "backwards") {
    feedingStepper.move(-steps);
  }
  
  while (feedingStepper.distanceToGo() != 0) {
    feedingStepper.run();
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
}

void loop() {
  if (feedingLimitReached()) {
    emergencyStop();
  }
  
  // Showcase moving to limits
  moveToLimits();
  
  // Uncomment to use reload function instead
  // reload();
}
