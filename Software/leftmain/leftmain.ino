#include <KeyboardBLE.h>

// Matrix config for left half
const int numCols = 5;
const int numRows = 4;

const int rowPins[numRows] = { 7, 13, 14, 15 };
const int colPins[numCols] = { 2, 3, 4, 5, 6 };

char keyMapLeft[numRows][numCols] = {
  { KEY_ESC, 'w', 'e', 'r', 't' },
  { 'q', 's', 'd', 'f', 'g' },
  { 'a', 'x', 'c', 'v', 'b' },
  { 'z', KEY_TAB, ' ', ' ', KEY_BACKSPACE }
};

char keyMapRight[numRows][numCols] = {
  { 'y', 'u', 'i', 'o', KEY_END },
  { 'h', 'j', 'k', 'l', 'p' },
  { 'n', 'm', ',', '.', ';' },
  { KEY_RETURN, ' ', '3', '4', '\\' }
};

bool lastKeyState[numRows][numCols] = { { false } };

// LED config
unsigned long previousLedMillis = 0;
bool ledOn = false; // current LED state
const unsigned long ledOnDuration = 100;
const unsigned long ledOffDuration = 3000;



void setup() {
  for (int r = 0; r < numRows; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }

  for (int c = 0; c < numCols; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  KeyboardBLE.begin();
  Serial1.begin(9600);
}

void loop() {
  // --- Left side matrix scan ---
  for (int r = 0; r < numRows; r++) {
    digitalWrite(rowPins[r], LOW);

    for (int c = 0; c < numCols; c++) {
      bool pressed = !digitalRead(colPins[c]);

      if (pressed && !lastKeyState[r][c]) {
        KeyboardBLE.press(keyMapLeft[r][c]);
      }
      if (!pressed && lastKeyState[r][c]) {
        KeyboardBLE.release(keyMapLeft[r][c]);
      }

      lastKeyState[r][c] = pressed;
    }

    digitalWrite(rowPins[r], HIGH);
  }

  // --- Right side UART handling ---
  while (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    int state, row, col;
    if (sscanf(line.c_str(), "%d,%d,%d", &state, &row, &col) == 3) {
      if (row >= 0 && row < numRows && col >= 0 && col < numCols) {
        char key = keyMapRight[row][col];
        if (state == 1) {
          KeyboardBLE.press(key);
        } else {
          KeyboardBLE.release(key);
        }
      }
    }
  }

  // --- LED Blink: 100 ms ON, 3000 ms OFF ---
  unsigned long currentMillis = millis();
  if (ledOn) {
    // LED is currently on; check if it has been on for at least 100 ms.
    if (currentMillis - previousLedMillis >= ledOnDuration) {
      digitalWrite(LED_BUILTIN, LOW);  // turn off LED
      previousLedMillis = currentMillis;
      ledOn = false;
    }
  } else {
    // LED is currently off; check if it has been off for at least 3000 ms.
    if (currentMillis - previousLedMillis >= ledOffDuration) {
      digitalWrite(LED_BUILTIN, HIGH); // turn on LED
      previousLedMillis = currentMillis;
      ledOn = true;
    }
  }

  delay(10);
}
