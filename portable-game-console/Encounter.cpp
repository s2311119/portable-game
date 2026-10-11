#include "Encounter.h"

#include <Arduino.h>

bool Encounter::checkEncounter() {
  stepCounter++;
  // 10歩〜30歩の間で確率的にエンカウント
  if (stepCounter > random(10, encounterThreshold)) {
    stepCounter = 0;
    return random(100) < 10; // 10% の確率で戦闘
  }
  return false;
}
