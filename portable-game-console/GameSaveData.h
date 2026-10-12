#pragma once

#include "Map.h"
#include "CharacterSaveData.h"
#include "InventorySaveData.h"

#include <stdint.h>
#include <vector>

enum class SaveFormat {
  LEGACY,
  PGSAVE2
};

struct GameSaveData {
  SaveFormat format = SaveFormat::PGSAVE2;
  int floor = 1;
  Map::MapType mapType = Map::TYPE_TOWN;
  int width = 0;
  int height = 0;
  int automatonWhoId = -1;
  int playerX = 0;
  int playerY = 0;
  int playerDirection = 0;
  std::vector<uint8_t> tiles;
  CharacterSaveData playerCharacter;
  InventorySaveData inventory;
  std::vector<CharacterSaveData> party;
  std::vector<CharacterSaveData> storage;
  bool bossDefeated[5] = {};
};
