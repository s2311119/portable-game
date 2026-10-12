#pragma once

#include <stdint.h>

class Controller {
public:
  const int BTN_UP=0, BTN_DOWN=3, BTN_LEFT=1, BTN_RIGHT=2;
  const int BTN_X=20, BTN_A=26, BTN_Y=27, BTN_B=21;

  // --- デバウンス用 ---
  const int btnPins[8] = {BTN_X, BTN_A, BTN_Y, BTN_B, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
  uint32_t lastPressTime[8];
  bool prevState[8];
  const uint32_t debounceMs = 180;

  Controller();

  // --- 通常押下（移動などに使う） ---
  bool pressed(int pin);

  bool pressedDebounced(int pin);
};
