#include <Arduino.h>

// ============================================================
// Arduino Uno IO pin scan test
//
// - Prints the state of all Uno IO pins every 3 seconds.
// - Uses one "next pin" button to cycle through the tested pin list.
// - Selected pin is shown with a marker in serial output.
//
// Wiring:
// - NEXT_PIN_BUTTON (D4) -> momentary button -> GND
//   (INPUT_PULLUP, so pressed == LOW)
// - To test a selected pin, connect that pin through your test button to GND.
//   Since pins are set to INPUT_PULLUP, pressed should read LOW.
// ============================================================

static constexpr uint8_t NEXT_PIN_BUTTON = 4;
static constexpr unsigned long PRINT_INTERVAL_MS = 3000;
static constexpr unsigned long DEBOUNCE_MS = 50;

// Uno digital numbering for analog pins:
// A0..A5 are D14..D19
static const uint8_t kPinsToScan[] = {
  2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4, A5
};
static constexpr size_t kPinCount = sizeof(kPinsToScan) / sizeof(kPinsToScan[0]);

static size_t selectedPinIndex = 0;
static bool prevNextPressed = false;
static unsigned long lastButtonEventMs = 0;
static unsigned long lastPrintMs = 0;

static inline bool isPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

static inline void printPinState(uint8_t pin, bool selected) {
  Serial.print(selected ? F(" > ") : F("   "));

  if (pin >= A0 && pin <= A5) {
    Serial.print(F("A"));
    Serial.print(pin - A0);
  } else {
    Serial.print(F("D"));
    Serial.print(pin);
  }

  Serial.print(F(": "));
  Serial.println(digitalRead(pin) == LOW ? F("LOW") : F("HIGH"));
}

void setup() {
  Serial.begin(9600);

  pinMode(NEXT_PIN_BUTTON, INPUT_PULLUP);

  for (size_t i = 0; i < kPinCount; i++) {
    pinMode(kPinsToScan[i], INPUT_PULLUP);
  }

  prevNextPressed = isPressed(NEXT_PIN_BUTTON);

  Serial.println(F("IO pin scan test started."));
  Serial.println(F("Press D4 button to select next pin."));
  Serial.println(F("Selected pin is prefixed with '>'"));
}

void loop() {
  const unsigned long now = millis();
  const bool nextPressed = isPressed(NEXT_PIN_BUTTON);

  if (nextPressed && !prevNextPressed) {
    if ((lastButtonEventMs == 0) || (now - lastButtonEventMs >= DEBOUNCE_MS)) {
      selectedPinIndex = (selectedPinIndex + 1) % kPinCount;
      lastButtonEventMs = now;
    }
  }
  prevNextPressed = nextPressed;

  if ((lastPrintMs == 0) || (now - lastPrintMs >= PRINT_INTERVAL_MS)) {
    lastPrintMs = now;
    Serial.println(F("--------------------------------"));
    for (size_t i = 0; i < kPinCount; i++) {
      printPinState(kPinsToScan[i], i == selectedPinIndex);
    }
  }
}
