#pragma once

#include <vector>
#include "Item.h"

class FsFile;

class Inventory {
public:
  // 所持品リスト
  struct ItemStack {
    Item item;
    int quantity;
  };
  std::vector<ItemStack> items;
  int money = 0;
  Inventory();

  // ★アイテムを追加する関数
  void addItem(const Item& newItem);

  int countItem(int id);

  bool removeItem(int id);
  void removeByIndex(int index);
  // (将来的に、アイテム一覧を表示する関数なども追加)
  void saveTo(FsFile& file);

  void loadFrom(FsFile& file);
};
