#pragma once

struct CoreStatsData {
  int maxHp;
  int maxTp;
  int attack;
  int defense;
  int speed;
  int sense;
  int luck;
};

class CharacterCatalog {
public:
  static CoreStatsData statsFor(int characterId, int level);
  static CoreStatsData initialStatsFor(int characterId, int level);
  static const char* nameFor(int characterId);
};
