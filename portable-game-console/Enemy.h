#pragma once
#include "CharacterState.h"

class Enemy{
public:
  const char* name;
  int who;
  int dropItemId;
  int dropRatePercent;
  int xpYield;

  Enemy(int w, int floor);
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
