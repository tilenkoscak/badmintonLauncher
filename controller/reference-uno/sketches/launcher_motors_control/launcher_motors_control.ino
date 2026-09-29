// ===============================
// DC motor test with soft start
// ===============================
// This sketch demonstrates:
//  1. Safe startup (guaranteed OFF)
//  2. Short 100% “kick” to overcome stall
//  3. Smooth PWM ramp up and down
//  4. Full stop at end of loop

const int motorPin = 5;      // PWM-capable pin (~)
const int kickDuration = 200; // Kick duration in milliseconds
const int baseDuty = 100;     // After kick, hold 40% duty (100/255 ≈ 39%)
const int maxDuty = 255;      // Full duty
const int stepDelay = 50;     // Delay between ramp steps in ms
const int stepSize = 10;      // How fast to ramp duty (smaller = smoother)

void setup() {
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);   // Ensure motor is OFF at power-up
  delay(2000);                   // Give you time to power the circuit
}

void loop() {
  // ---------- Kick start ----------
  analogWrite(motorPin, maxDuty);    // Full power for a short time
  delay(kickDuration);               // Let motor start spinning

  // ---------- Drop to lower base speed ----------
  analogWrite(motorPin, baseDuty);
  delay(1000);                       // Hold for 1s

  // ---------- Ramp up from 40% → 100% ----------
  for (int duty = baseDuty; duty <= maxDuty; duty += stepSize) {
    analogWrite(motorPin, duty);
    delay(stepDelay);
  }

  // ---------- Ramp down from 100% → 40% ----------
  for (int duty = maxDuty; duty >= baseDuty; duty -= stepSize) {
    analogWrite(motorPin, duty);
    delay(stepDelay);
  }

  // ---------- Stop motor ----------
  analogWrite(motorPin, 0);
  delay(3000);  // Motor fully stops before repeating
}
