#include <Keyboard.h>
#include <KeyboardBLE.h>
#include <Mouse.h>

// #include <KeyboardBLE.h>
extern "C" void reset_usb_boot(uint32_t usb_activity_gpio_pin_mask, uint32_t disable_interface_mask);

bool ledOn = false;        // current LED state
uint8_t count = 0;         // counts pressed keys
bool game = 0;
bool h = 0;

const uint8_t COLS = 5;                       // no of cols
const uint8_t ROWS = 4;                       // no of rows
const int rowPins[ROWS] = { 7, 13, 14, 15 };  // row gpio map
const int colPins[COLS] = { 2, 3, 4, 5, 6 };  // col gpio map

// state arrays: [half][row][col]
static bool state[2][ROWS][COLS]   = {{false}};
static bool hold[2][ROWS][COLS]    = {{false}};
static bool tapSent[2][ROWS][COLS] = {{false}};
static uint32_t tyme[2][ROWS][COLS] = {{0}};

unsigned long now;
static const unsigned long htyme = 200;  // hold threshold in ms

void key(bool pressed, uint8_t code);
void led(uint8_t count);

enum Layer {
  BASE = 0,
  ONE,
  TWO,
  THREE
};
uint8_t current = BASE;

enum Mode {
  TAP_ONLY   = 0,
  HOLD_MOD   = 1,
  LAYER_MOD  = 2,
  MACRO      = 3
};

enum MacroID {
  M_SWITCH = 1,
  M_PASS   = 2,
  M_UP     = 3,
  M_DOWN   = 4,
  M_LEFT   = 5,
  M_RIGHT  = 6,
  M_CLK    = 7,
  M_RCLK   = 8,
};

struct KeyDef {
  uint8_t mode;  // TAP_ONLY, HOLD_MOD, LAYER_MOD, MACRO
  uint8_t tap;   // keycode on tap
  uint8_t alt;   // modifier or layer or macro ID
};

// Left and right key maps [layer][row][col]
static const KeyDef lMap[THREE][ROWS][COLS] = {
  // BASE layer
  {
    {{0, KEY_ESC, 0}, {0, 'w', 0}, {0, 'f', 0}, {0, 'p', 0}, {0, 'b', 0}},
    {{0, 'q', 0}, {1, 'r', KEY_LEFT_ALT}, {1, 's', KEY_LEFT_SHIFT}, {1, 't', KEY_LEFT_CTRL}, {0, 'g', 0}},
    {{1, 'a', KEY_LEFT_GUI}, {0, 'x', 0}, {0, 'c', 0}, {0, 'd', 0}, {0, 'v', 0}},
    {{0, 'z', 0}, {0, KEY_TAB, 0}, {0, KEY_LEFT_ALT, 0}, {2, ' ', ONE}, {2, KEY_BACKSPACE, TWO}}
  },
  // FN layer
  {
    {{0, KEY_F1, 0}, {0, KEY_F2, 0}, {0, KEY_F3, 0}, {0, KEY_PAGE_UP, 0}, {0, KEY_PAGE_DOWN, 0}},
    {{0, KEY_HOME, 0}, {0, KEY_LEFT_ARROW, 0}, {0, KEY_UP_ARROW, 0}, {0, KEY_RIGHT_ARROW, 0}, {0, KEY_END, 0}},
    {{0, KEY_F11, 0}, {0, KEY_F12, 0}, {0, KEY_DOWN_ARROW, 0}, {3, 0, M_UP}, {3, 'k', M_SWITCH}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {2, 0, ONE}, {2, KEY_BACKSPACE, TWO}}
  },
  {
    {{0, KEY_F1, 0}, {0, KEY_F2, 0}, {0, KEY_F3, 0}, {0, KEY_PAGE_UP, 0}, {0, KEY_PAGE_DOWN, 0}},
    {{0, KEY_HOME, 0}, {0, KEY_LEFT_ARROW, 0}, {0, KEY_UP_ARROW, 0}, {0, KEY_RIGHT_ARROW, 0}, {0, KEY_END, 0}},
    {{0, KEY_F11, 0}, {0, KEY_F12, 0}, {0, KEY_DOWN_ARROW, 0}, {3, 0, M_UP}, {3, 'k', M_SWITCH}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {2, 0, ONE}, {2, KEY_BACKSPACE, TWO}}
  }
};

static const KeyDef rMap[THREE][ROWS][COLS] = {
  // BASE layer
  {
    {{1, 'j', '1'}, {0, 'l', 0}, {0, 'u', 0}, {0, 'y', 0}, {0, KEY_DELETE, 0}},
    {{0, 'm', 0}, {1, 'n', KEY_LEFT_CTRL}, {1, 'e', KEY_LEFT_SHIFT}, {1, 'i', KEY_LEFT_ALT}, {0, ';', 0}},
    {{0, 'k', 0}, {0, 'h', 0}, {0, ',', 0}, {0, '.', 0}, {1, 'o', KEY_RIGHT_GUI}},
    {{0, KEY_RETURN, 0}, {0, ' ', 0}, {0, KEY_UP_ARROW, 0}, {0, KEY_RIGHT_ARROW, 0}, {0, '/', 0}}
  },
  // FN layer
  {
    {{0, 'j', 0}, {0, '1', 0}, {0, '2', 0}, {0, '3', 0}, {0, KEY_DELETE, 0}},
    {{0, 'm', 0}, {3, '4', M_LEFT}, {3, '5', M_UP}, {3, '6', M_RIGHT}, {0, ';', 0}},
    {{0, 'k', 0}, {0, '7', 0}, {3, '8', M_DOWN}, {0, '9', 0}, {0, 'o', 0}},
    {{3, 0, M_RCLK}, {3, 0, M_CLK}, {0, KEY_UP_ARROW, 0}, {0, KEY_RIGHT_ARROW, 0}, {0, '0', 0}}
  },
  {
    {{0, 'j', 0}, {0, '1', 0}, {0, '2', 0}, {0, '3', 0}, {0, KEY_DELETE, 0}},
    {{0, 'm', 0}, {0, '4', M_LEFT}, {0, '5', M_UP}, {0, '6', M_RIGHT}, {0, ';', 0}},
    {{0, 'k', 0}, {0, '7', 0}, {0, '8', M_DOWN}, {0, '9', 0}, {0, 'o', 0}},
    {{0, 0, M_RCLK}, {0, 0, M_CLK}, {0, KEY_UP_ARROW, 0}, {0, KEY_RIGHT_ARROW, 0}, {0, '0', 0}}
  }
};

void setup() {
  for (int r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }
  for (int c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial1.begin(115200);
  Mouse.begin();
  KeyboardBLE.begin();
}

void loop() {
  // --- Left side matrix scan ---
  for (uint8_t r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    for (uint8_t c = 0; c < COLS; c++) {
      bool pressed = (digitalRead(colPins[c]) == LOW);
      h   = 0;
      now = millis();
      const KeyDef &K = lMap[current][r][c];

      if (pressed) {
        // TAP_ONLY
        if (game || K.mode == TAP_ONLY) {
          if (!state[h][r][c]) {
            key(true, K.tap);
            state[h][r][c] = true;
          }
        }
        // HOLD_MOD
        else if (K.mode == HOLD_MOD) {
          if (!state[h][r][c]) {
            state[h][r][c]    = true;
            tyme[h][r][c]     = now;
            hold[h][r][c]     = false;
            tapSent[h][r][c]  = false;  // clear quick‑tap flag
          }
          else if (!hold[h][r][c] && now - tyme[h][r][c] >= htyme) {
            key(true, K.alt);
            hold[h][r][c] = true;
          }
        }
        // LAYER_MOD
        else if (K.mode == LAYER_MOD) {
          if (!state[h][r][c]) {
            state[h][r][c]    = true;
            tyme[h][r][c]     = now;
            hold[h][r][c]     = false;
            tapSent[h][r][c]  = false;
          }
          else if (!hold[h][r][c] && now - tyme[h][r][c] >= htyme) {
            current = K.alt;
            hold[h][r][c] = true;
          }
        }
        // MACRO
        else if (K.mode == MACRO) {
          if (!state[h][r][c]) {
            runMacro(K.alt);
            state[h][r][c] = true;
          }
        }
      }
      else {
        // TAP_ONLY release
        if (game || K.mode == TAP_ONLY) {
          if (state[h][r][c]) {
            key(false, K.tap);
            state[h][r][c] = false;
          }
        }
        // HOLD_MOD release
        else if (K.mode == HOLD_MOD) {
          if (state[h][r][c]) {
            if (!hold[h][r][c]
                && (now - tyme[h][r][c] < htyme)
                && !tapSent[h][r][c]) {
              key(true,  K.tap);
              key(false, K.tap);
              tapSent[h][r][c] = true;  // prevent repeats
            }
            else if (hold[h][r][c]) {
              key(false, K.alt);
            }
            state[h][r][c] = false;
            hold[h][r][c]  = false;
          }
        }
        // LAYER_MOD release
        else if (K.mode == LAYER_MOD) {
          if (state[h][r][c]) {
            if (!hold[h][r][c]
                && (now - tyme[h][r][c] < htyme)
                && !tapSent[h][r][c]) {
              key(true,  K.tap);
              key(false, K.tap);
              tapSent[h][r][c] = true;
            }
            else if (hold[h][r][c]) {
              current = BASE;
            }
            state[h][r][c] = false;
            hold[h][r][c]  = false;
          }
        }
        // MACRO release
        else if (K.mode == MACRO) {
          if (state[h][r][c]) {
            state[h][r][c] = false;
          }
        }
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }

  // --- Right side UART handling (non‑blocking) ---
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    serial(line);
  }

  // --- Right‑half timed hold & layer check ---
  now = millis();
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      if (state[1][r][c] && !hold[1][r][c] && (now - tyme[1][r][c] >= htyme)) {
        KeyDef K = rMap[current][r][c];
        if (K.mode == HOLD_MOD) {
          key(true, K.alt);
          hold[1][r][c] = true;
        }
        else if (K.mode == LAYER_MOD) {
          current = K.alt;
          hold[1][r][c] = true;
        }
      }
    }
  }

  // --- Bootsel combo check ---
  if (state[0][0][0] && state[0][0][1] && state[0][0][2]) {
    Keyboard.releaseAll();
    reset_usb_boot(0, 0);
  }

  delay(10);
}

// UART edge handler
void serial(String line) {
  int pressed, r, c;
  now = millis();
  h   = 1;
  if (sscanf(line.c_str(), "%d,%d,%d", &pressed, &r, &c) == 3) {
    const KeyDef &K = rMap[current][r][c];
    if (pressed) {
      if (game || K.mode == TAP_ONLY) {
        if (!state[h][r][c]) {
          key(true, K.tap);
          state[h][r][c] = true;
        }
      }
      else if (K.mode == HOLD_MOD) {
        if (!state[h][r][c]) {
          state[h][r][c]   = true;
          tyme[h][r][c]    = now;
          hold[h][r][c]    = false;
          tapSent[h][r][c] = false;
        }
      }
      else if (K.mode == LAYER_MOD) {
        if (!state[h][r][c]) {
          state[h][r][c]   = true;
          tyme[h][r][c]    = now;
          hold[h][r][c]    = false;
          tapSent[h][r][c] = false;
        }
      }
      else if (K.mode == MACRO) {
        if (!state[h][r][c]) {
          runMacro(K.alt);
          state[h][r][c] = true;
        }
      }
    }
    else {
      if (game || K.mode == TAP_ONLY) {
        if (state[h][r][c]) {
          key(false, K.tap);
          state[h][r][c] = false;
        }
      }
      else if (K.mode == HOLD_MOD) {
        if (state[h][r][c]) {
          if (!hold[h][r][c]
              && (now - tyme[h][r][c] < htyme)
              && !tapSent[h][r][c]) {
            key(true,  K.tap);
            key(false, K.tap);
            tapSent[h][r][c] = true;
          }
          else if (hold[h][r][c]) {
            key(false, K.alt);
          }
          state[h][r][c] = false;
          hold[h][r][c]  = false;
        }
      }
      else if (K.mode == LAYER_MOD) {
        if (state[h][r][c]) {
          if (!hold[h][r][c]
              && (now - tyme[h][r][c] < htyme)
              && !tapSent[h][r][c]) {
            key(true,  K.tap);
            key(false, K.tap);
            tapSent[h][r][c] = true;
          }
          else if (hold[h][r][c]) {
            current = BASE;
          }
          state[h][r][c] = false;
          hold[h][r][c]  = false;
        }
      }
      else if (K.mode == MACRO) {
        if (state[h][r][c]) {
          state[h][r][c] = false;
        }
      }
    }
  }
}

// Macro runner
void runMacro(uint8_t id) {
  switch (id) {
    case M_SWITCH:
      key(1, KEY_LEFT_ALT);
      key(1, KEY_TAB);
      key(0, KEY_TAB);
      key(0, KEY_LEFT_ALT);
      break;
    case M_PASS:
      Keyboard.print("2424");
      break;
    case M_UP:
      Mouse.move(0, -20);
      break;
    case M_DOWN:
      Mouse.move(0, 20);
      break;
    case M_LEFT:
      Mouse.move(-20, 0);
      break;
    case M_RIGHT:
      Mouse.move(20, 0);
      break;
    case M_CLK:
      Mouse.click(MOUSE_LEFT);
      break;
    case M_RCLK:
      Mouse.click(MOUSE_RIGHT);
      break;
  }
}

void key(bool pressed, uint8_t code) {
  if (pressed) {
    Keyboard.press(code);
        KeyboardBLE.press(code);

    count++;
  } else {
    Keyboard.release(code);
        KeyboardBLE.release(code);

    count--;
  }
  led(count);
}

void led(uint8_t val) {
  digitalWrite(LED_BUILTIN, val > 0 ? HIGH : LOW);
}
