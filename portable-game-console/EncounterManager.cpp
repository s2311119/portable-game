#include "EncounterManager.h"

#include "Map.h"

bool EncounterManager::onStep(const Map& map, const EncounterRandom& random) {
  if (map.isTown()) return false;
  if (map.isBossRoom()) return false;

  stepCounter++;
  // 10歩〜30歩の間で確率的にエンカウント
  if (stepCounter > random.drawRange(10, encounterThreshold)) {
    stepCounter = 0;
    return random.drawMax(100) < 10; // 10% の確率で戦闘
  }
  return false;
}
