#include "Vibration.h"
#include <Arduino.h>

void Vibration::init() {
    pinMode(VIB_PIN, OUTPUT);
    digitalWrite(VIB_PIN, LOW);
  }

void Vibration::trigger(int ms) {
    if (!_enabled) return;
    digitalWrite(VIB_PIN, HIGH);
    vibrationEndTime = millis() + ms;
  }

void Vibration::setEnabled(bool enabled) {
    _enabled = enabled;
  }

void Vibration::update() {
    if (vibrationEndTime > 0 && millis() > vibrationEndTime) {
      digitalWrite(VIB_PIN, LOW);
      vibrationEndTime = 0;
    }
  }
