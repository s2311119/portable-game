#pragma once

class Status;

class Enemy{
public:
  Status* status;
  const char* name;
  int who;
  int dropItemId;
  int dropRatePercent;
  int xpYield;

  Enemy(int w, int floor);

  ~Enemy();
};
