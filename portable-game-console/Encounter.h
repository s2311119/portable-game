#pragma once

class Encounter {
public:
  int stepCounter = 0;
  const int encounterThreshold = 20; // 歩数間隔の平均
  bool checkEncounter();
};
