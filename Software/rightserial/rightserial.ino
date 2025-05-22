
// Matrix dimensions
static const uint8_t ROWS = 4;
static const uint8_t COLS = 5;

// Pin assignments
static const uint8_t rowPins[ROWS] = {17, 16, 15, 14};      // outputs
static const uint8_t colPins[COLS] = {13, 12, 11, 10, 9};   // inputs w/ pullups

static const unsigned long BAUD = 115200;

// Per-key state to detect edges
bool keyState[ROWS][COLS] = {};

void setup() {
  // rows as outputs (inactive HIGH)
  for (uint8_t r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }
  // cols as inputs with pullups
  for (uint8_t c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
Serial1.setRX(1);
Serial1.setTX(0);
  Serial1.begin(BAUD);
}

void loop() {
  for (uint8_t r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    for (uint8_t c = 0; c < COLS; c++) {
      bool pressed = (digitalRead(colPins[c]) == LOW);
      if (pressed != keyState[r][c]) {
        keyState[r][c] = pressed;
        // send "state,row,col\n"
        Serial1.print(pressed ? '1' : '0');
        Serial1.print(',');
        Serial1.print(r);
        Serial1.print(',');
        Serial1.println(c);
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  delay(10);  // debounce
}
