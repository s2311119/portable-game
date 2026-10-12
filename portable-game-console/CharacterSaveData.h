#pragma once

#include <vector>

struct CharacterSaveData {
  int characterId = 0;
  int level = 0;
  int xp = 0;
  int hp = 0;
  int tp = 0;
  int weaponId = 0;
  int armorId = 0;
  int accessoryId = 0;
  std::vector<int> skillIds;
};
