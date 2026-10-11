#pragma once

enum class CombatantSide {
  PLAYER,
  ALLY,
  ENEMY
};

struct CombatantRef {
  CombatantSide side;
  int index;
};
