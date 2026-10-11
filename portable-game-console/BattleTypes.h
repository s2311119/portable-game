#pragma once

#include "EncounterSpec.h"

enum class CombatantSide {
  PLAYER,
  ALLY,
  ENEMY
};

struct CombatantRef {
  CombatantSide side;
  int index;
};

enum class BattleOutcome {
  VICTORY,
  DEFEAT,
  ESCAPE
};

struct BattleResult {
  BattleOutcome outcome;
  BattleKind kind;
};
