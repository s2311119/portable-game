#pragma once
#include "CharacterState.h"

class Enemy{
public:
  const char* name;
  int who;
  int dropItemId;
  int dropRatePercent;
  int xpYield;

  // rolledLevel is the already-drawn level for XP; rare/Boss state presets
  // continue to use their existing initial levels.
  static Enemy fromRolledLevel(int requestedId, int rolledLevel);
  Enemy(const Enemy&) = delete;
  Enemy& operator=(const Enemy&) = delete;
  Enemy(Enemy&&) noexcept;
  Enemy& operator=(Enemy&&) noexcept;

  CharacterState& state();
  const CharacterState& state() const;

  ~Enemy();

private:
  struct StateInitTag {};
  Enemy(int requestedId, int rolledLevel, StateInitTag);
  CharacterState stateValue;
};
