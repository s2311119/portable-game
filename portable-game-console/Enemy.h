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

  CharacterState& state();
  const CharacterState& state() const;

  ~Enemy();

private:
  struct StateInitTag {};
  Enemy(int requestedId, int rolledLevel, StateInitTag);
  CharacterState stateValue;
};
