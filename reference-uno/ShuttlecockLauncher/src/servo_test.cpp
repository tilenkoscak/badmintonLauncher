#include <Arduino.h>
#include <Servo.h>

#define SERVO_PIN 9

// Buttons: D4=dec, D5=inc. If D11 always reads 0 on your board, the pin is held LOW (wiring/shield/damage)—not this sketch.
// D10/D11 are not taken over by the Uno Servo lib’s Timer1 for digital input.
// on a Uno (it only uses compare interrupts; it does not take over D10/D11 as inputs).
// If D10/D11 never read LOW when you press but D4/D5 do, treat it as wiring/header/pin
// hardware, not this sketch.

// INPUT_PULLUP: each button from pin to GND; pressed reads LOW
#define SERVO_DEC_BUTTON_PIN 4
#define SERVO_INC_BUTTON_PIN 5

static constexpr unsigned long kDebounceMs = 50;
static constexpr unsigned long kSerialDiagMs = 800;

static Servo myservo;
static int pos = 90;

static bool prevIncPressed = false;
static bool prevDecPressed = false;
static unsigned long lastIncStepMs = 0;
static unsigned long lastDecStepMs = 0;
static unsigned long lastSerialMs = 0;

static inline bool rawPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

void setup() {
  Serial.begin(9600);
  pinMode(SERVO_DEC_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SERVO_INC_BUTTON_PIN, INPUT_PULLUP);

  prevIncPressed = rawPressed(SERVO_INC_BUTTON_PIN);
  prevDecPressed = rawPressed(SERVO_DEC_BUTTON_PIN);

  myservo.attach(SERVO_PIN);
  myservo.write(pos);
  delay(1000);

  Serial.println(F("Servo on D9; buttons D4=dec D5=inc (INPUT_PULLUP, pressed=LOW)"));
}

void loop() {
  const unsigned long now = millis();
  const bool incPressed = rawPressed(SERVO_INC_BUTTON_PIN);
  const bool decPressed = rawPressed(SERVO_DEC_BUTTON_PIN);

  if (now - lastSerialMs >= kSerialDiagMs) {
    lastSerialMs = now;
    Serial.print(F("D4="));
    Serial.print(digitalRead(SERVO_DEC_BUTTON_PIN));
    Serial.print(F(" D5="));
    Serial.print(digitalRead(SERVO_INC_BUTTON_PIN));
    Serial.print(F(" pos="));
    Serial.println(pos);
  }

  if (incPressed && !prevIncPressed && pos < 180) {
    if (lastIncStepMs == 0 || (now - lastIncStepMs) >= kDebounceMs) {
      pos++;
      myservo.write(pos);
      lastIncStepMs = now;
    }
  }
  if (decPressed && !prevDecPressed && pos > 0) {
    if (lastDecStepMs == 0 || (now - lastDecStepMs) >= kDebounceMs) {
      pos--;
      myservo.write(pos);
      lastDecStepMs = now;
    }
  }

  prevIncPressed = incPressed;
  prevDecPressed = decPressed;

  // for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
  //   // in steps of 1 degree
  //   myservo.write(pos);              // tell servo to go to position in variable 'pos'
  //   delay(15);                       // waits 15 ms for the servo to reach the position
  // }
  // for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
  //   myservo.write(pos);              // tell servo to go to position in variable 'pos'
  //   delay(15);                       // waits 15 ms for the servo to reach the position
  // }
}
