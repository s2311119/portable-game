#include "Inventory.h"
#include "ItemCatalog.h"


Inventory::Inventory() {
    // 起動時に std::vector のメモリを
    // ある程度確保しておくと、後々の追加処理が高速になります
    items.reserve(20); // とりあえず20個分
}

void Inventory::addItem(const Item& newItem) {
    // 既に持っているか探す
    for (auto& stack : items) {
      if (stack.item.id == newItem.id) {
        stack.quantity++; // あったら個数を増やす
        return;
      }
    }
    // なかったら新しく追加 (個数1)
    items.push_back({newItem, 1});
}

int Inventory::countItem(int id) {
    for (const auto& stack : items) {
      if (stack.item.id == id) {
        return stack.quantity;
      }
    }
    return 0;
}

bool Inventory::removeItem(int id) {
    for (auto it = items.begin(); it != items.end(); ++it) {
      if (it->item.id == id) {
        it->quantity--;
        if (it->quantity <= 0) {
          items.erase(it); // 0個になったらリストから消す
        }
        return true;
      }
    }
    return false; // 見つからなかった
}

void Inventory::removeByIndex(int index) {
      if (index < 0 || index >= items.size()) return;
      
      items[index].quantity--;
      if (items[index].quantity <= 0) {
          items.erase(items.begin() + index);
      }
}

InventorySaveData Inventory::snapshotPersistentState() const {
    InventorySaveData saved;
    saved.money = money;
    for (const auto& stack : items) {
      saved.items.push_back({stack.item.id, stack.quantity});
    }
    return saved;
}

void Inventory::restorePersistentState(const InventorySaveData& saved) {
    money = saved.money;
    items.clear();
    for (const auto& entry : saved.items) {
      // アイテム生成してリストに追加
      Item item = ItemCatalog::get(entry.itemId);
      items.push_back({item, entry.quantity});
    }
}
