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

EncounterSpec EncounterManager::createEncounter(
    int floor, const Map& map, const EncounterRandom& random) const {
  if (floor >= 10 &&
      floor <= 50 &&
      floor % 10 == 0 &&
      !map.isTown()) {
    return buildBossEncounter(floor, random);
  }
  return createNormalEncounter(floor, random);
}

EncounterSpec EncounterManager::createBossEncounter(
    int floor, const Map& map, const EncounterRandom& random) const {
  // Preserve the old Battle::start fallback for restored non-Boss maps.
  return createEncounter(floor, map, random);
}

int EncounterManager::rollLevel(int floor, const EncounterRandom& random) {
  int level = floor + random.drawRange(0, 3);
  if (level < 1) level = 1;
  return level;
}

EncounterSpec EncounterManager::buildBossEncounter(
    int floor, const EncounterRandom& random) const {
  EncounterSpec spec;
  spec.kind = floor == 50 ? BattleKind::FINAL_BOSS : BattleKind::BOSS;

  int bossId = 0;
  if (floor == 10) bossId = 80;
  else if (floor == 20) bossId = 81;
  else if (floor == 30) bossId = 82;
  else if (floor == 40) bossId = 83;
  else if (floor == 50) bossId = 84;

  // The old Enemy(bossId, 1) constructor consumed this roll even though
  // the boss's initial CharacterState level is fixed at 1.
  spec.enemies.push_back({bossId, rollLevel(1, random)});
  return spec;
}

EncounterSpec EncounterManager::createNormalEncounter(
    int floor, const EncounterRandom& random) const {
  EncounterSpec spec;
  spec.kind = BattleKind::NORMAL;

  std::vector<int> candidates;
  bool rare_36_appears = false;
  bool rare_37_appears = false;
  if (floor <= 10) {
    candidates = {0, 1, 2, 3, 4};
  } else if (floor <= 20) {
    candidates = {5, 6, 7, 8, 9};
  } else if (floor <= 30) {
    candidates = {10, 11, 12, 13, 14};
    rare_36_appears = true;
  } else if (floor <= 40) {
    candidates = {15, 17, 19, 20, 21};
    rare_36_appears = true;
    rare_37_appears = true;
  } else {
    candidates = {22, 24, 26, 28, 30, 32, 34};
    rare_36_appears = true;
    rare_37_appears = true;
  }

  int enemyCount = random.drawRange(1, 5);
  for (int i = 0; i < enemyCount; i++) {
    int who = 0;
    int r = random.drawMax(100);

    if (rare_37_appears && r < 3) {
      who = 37;
    } else if (rare_36_appears && r < 8) {
      who = 36;
    } else if (floor <= 20 && random.drawMax(100) < 3) {
      who = 22;
    } else if (floor <= 40 && floor > 20 && random.drawMax(100) < 3) {
      who = 24;
    } else {
      if (!candidates.empty()) {
        int idx = random.drawRange(0, candidates.size());
        who = candidates[idx];
      } else {
        who = 0;
      }
    }

    // The previous Enemy constructor rolled immediately after each ID.
    spec.enemies.push_back({who, rollLevel(floor, random)});
  }
  return spec;
}
