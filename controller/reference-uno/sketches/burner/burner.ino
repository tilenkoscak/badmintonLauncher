/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 https://www.arduino.cc/en/Tutorial/LibraryExamples/Sweep
*/

#include <Servo.h>

Servo armsServo;  // create servo object to control a servo
Servo spoonServo;
// twelve servo objects can be created on most boards

int pos = 0;
int loop_nr = 0;    // variable to store the servo position

// Define button pins
#define BUTTON_STOP_PIN 4  // Button for clockwise rotation

void setup() {
  armsServo.attach(9);  // attaches the servo on pin 9 to the servo object
  spoonServo.attach(10);
  armsServo.write(10);
  spoonServo.write(180);
  delay(1000);

  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);
}

void loop() {
  // for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
  //   // in steps of 1 degree
  //   armsServo.write(pos);              // tell servo to go to position in variable 'pos'
  //   delay(30);                       // waits 15 ms for the servo to reach the position
  // }
  // for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
  //   armsServo.write(pos);              // tell servo to go to position in variable 'pos'
  //   delay(15);                       // waits 15 ms for the servo to reach the position
  // }
  // //closed
  // armsServo.write(17);
  // delay(1500);
  // //open
  // armsServo.write(40);
  // delay(1500);

  //closed
  spoonServo.write(180);
  delay(15000); 
  //open
  spoonServo.write(0);
  delay(15000);

  // if (loop_nr < 3) {
  //   armsServo.write(40);
  //   delay(1000);
  //   armsServo.write(10);
  //   delay(1000);
  //   spoonServo.write(90);
  //   delay(1000);
  //   spoonServo.write(180);
  //   delay(1000);
  // } else {
  //   armsServo.write(40);
  //   spoonServo.write(90);
  //   while(1);
  // }
  // loop_nr = loop_nr + 1;

  // bool stopButtonState = digitalRead(BUTTON_STOP_PIN);

  // if (stopButtonState == LOW) {
  //   armsServo.write(40);
  //   delay(1000);
  //   armsServo.write(10);
  //   delay(1000);
  //   spoonServo.write(90);
  //   delay(1000);
  //   spoonServo.write(180);
  //   delay(1000);
  // } else {
  //   armsServo.write(40);
  //   spoonServo.write(90);
  //   while(1);
  // }

  // add button 5 to reset the cycle

}
