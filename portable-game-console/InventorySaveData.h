#pragma once

#include <vector>

struct InventoryItemSaveData {
  int itemId = 0;
  int quantity = 0;
};

struct InventorySaveData {
  int money = 0;
  std::vector<InventoryItemSaveData> items;
};
