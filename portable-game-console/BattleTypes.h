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

enum class BattleBgmEvent {
  START,
  EARLY_FIELD_RETURN,
  FINAL_FIELD_RETURN,
  DEFEAT
};

struct BattleBgmHooks {
  void* context;
  void (*onEvent)(void* context, BattleBgmEvent event, BattleKind kind);

  void emit(BattleBgmEvent event, BattleKind kind) const {
    onEvent(context, event, kind);
  }
};
