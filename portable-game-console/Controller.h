#pragma once

#include <stdint.h>
#include "LegacyGameTypes.h"

class Caractor;
class Map;

class Controller {
public:
  const int BTN_UP=0, BTN_DOWN=3, BTN_LEFT=1, BTN_RIGHT=2;
  const int BTN_X=20, BTN_A=26, BTN_Y=27, BTN_B=21;
  uint32_t lastMoveTime = 0;
  const uint32_t moveIntervalMs = 80;

  // --- デバウンス用 ---
  const int btnPins[8] = {BTN_X, BTN_A, BTN_Y, BTN_B, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
  uint32_t lastPressTime[8];
  bool prevState[8];
  const uint32_t debounceMs = 180;

  Controller();

  // --- 通常押下（移動などに使う） ---
  bool pressed(int pin);

  bool pressedDebounced(int pin);
  
  // --- キャラ移動 ---
  void update_cara_point(Caractor &c);

  // --- 壁判定 ---
  void check_wall(Caractor &hero, Map &map);

  bool check_event(Caractor &hero, Map &map, GameState &state, EventType &currentEvent);
};
