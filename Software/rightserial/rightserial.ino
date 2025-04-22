#include <KeyboardBLE.h>
#include <Keyboard.h>

// BLE keyboard instance

// Matrix dimensions
const uint8_t ROWS = 4;
const uint8_t COLS = 5;

// Pin assignments (adjust as needed)
const uint8_t rowPins[ROWS] = {17, 16, 15, 14};       // output
const uint8_t colPins[COLS] = {13, 12, 11, 10, 9};   // input_pullup

// Key HID codes
const uint8_t keyMap[ROWS][COLS] = {
  {'j', 'l', 'u',  'y',  KEY_END},
  {'m', 'n', 'e',  'i',  ';'},
  {'k', 'h', ',',  '.',  'o'},
  {KEY_RETURN, ' ', KEY_UP_ARROW,  KEY_RIGHT_ARROW,  '/'}
};
// LED config
unsigned long previousLedMillis = 0;
bool ledOn = false;
const unsigned long ledOnDuration = 1;
const unsigned long ledOffDuration = 1000;

// Track current key state
bool keyState[ROWS][COLS] = { false };

void setup() {
  // Matrix setup
  for (uint8_t r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }
  for (uint8_t c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

    pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Start BLE keyboard
  KeyboardBLE.begin();

  // Start Serial1 UART (default TX = GPIO4, RX = GPIO5)
  Serial1.begin(115200);
}

void loop() {

  for (uint8_t r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);  // activate row

    for (uint8_t c = 0; c < COLS; c++) {
      bool pressed = (digitalRead(colPins[c]) == LOW);

      if (pressed != keyState[r][c]) {
        keyState[r][c] = pressed;

        if (pressed) {
          KeyboardBLE.press(keyMap[r][c]);
          Keyboard.press(keyMap[r][c]);
          Serial1.print(pressed ? "1" : "0");
          Serial1.print(",");
          Serial1.println(keyMap[r][c]);
        } else {
          KeyboardBLE.release(keyMap[r][c]);
          Keyboard.release(keyMap[r][c]);nneninineieinieneineenninneeininnee
          Serial1.print(pressed ? "1" : "0");
          Serial1.print(",");
          Serial1.println(keyMap[r][c]);
        }
      }
    }

    digitalWrite(rowPins[r], HIGH);  // deactivate row
  }


  unsigned long currentMillis = millis();
  if (ledOn) {
    if (currentMillis - previousLedMillis >= ledOnDuration) {
      digitalWrite(LED_BUILTIN, LOW);
      previousLedMillis = currentMillis;
      ledOn = false;
    }
  } else {
    if (currentMillis - previousLedMillis >= ledOffDuration) {
      digitalWrite(LED_BUILTIN, HIGH);
      previousLedMillis = currentMillis;
      ledOn = true;
    }
  }


  delay(10);  // scan debounce
}
