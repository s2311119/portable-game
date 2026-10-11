#pragma once

#include <vector>

enum class BattleKind {
  NORMAL,
  BOSS,
  FINAL_BOSS
};

struct EnemySpawnSpec {
  int enemyId;
  int level;
};

struct EncounterSpec {
  std::vector<EnemySpawnSpec> enemies;
  BattleKind kind;
};
