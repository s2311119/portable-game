#include "Controller.h"
#include <Arduino.h>

Controller::Controller() {
    for (int i = 0; i < 8; ++i) {
      pinMode(btnPins[i], INPUT_PULLUP);
      lastPressTime[i] = 0;
      // 押下状態（LOW）をboolで保持
      prevState[i] = (digitalRead(btnPins[i]) == LOW);
    }
  }

bool Controller::pressed(int pin) {
    return digitalRead(pin) == LOW;
  }

bool Controller::pressedDebounced(int pin) {
    // find index
    int idx = -1;
    for (int i = 0; i < 8; ++i) if (btnPins[i] == pin) { idx = i; break; }
    if (idx < 0) return false; // unknown pin

    bool isPressed = (digitalRead(pin) == LOW);
    if (isPressed) {
      uint32_t now = millis();
      if (now - lastPressTime[idx] > debounceMs) {
        lastPressTime[idx] = now;
        return true;
      }
    }
    return false;
  }
