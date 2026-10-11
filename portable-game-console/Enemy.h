#pragma once

class CharacterState;

class Enemy{
public:
  CharacterState* status;
  const char* name;
  int who;
  int dropItemId;
  int dropRatePercent;
  int xpYield;

  Enemy(int w, int floor);

  ~Enemy();
};
