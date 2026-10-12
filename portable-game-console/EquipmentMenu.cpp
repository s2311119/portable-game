#include "EquipmentMenu.h"

#include "Inventory.h"

void EquipmentMenu::open() {
  state_ = State::SLOT;
  cursor_ = 0;
}

void EquipmentMenu::moveUp(int count) {
  cursor_ = (cursor_ - 1 + count) % count;
}

void EquipmentMenu::moveDown(int count) {
  cursor_ = (cursor_ + 1) % count;
}

void EquipmentMenu::selectSlot() {
  selectedSlot_ = cursor_;
  state_ = State::CANDIDATE;
  cursor_ = 0;
}

std::vector<int> EquipmentMenu::candidateIndices(const Inventory& inventory) const {
  std::vector<int> indices;
  Item::ItemType targetType = Item::ITEM_WEAPON;
  if (selectedSlot_ == 1) targetType = Item::ITEM_ARMOR;
  if (selectedSlot_ == 2) targetType = Item::ITEM_ACCESSORY;

  for (int i = 0; i < inventory.items.size(); i++) {
    if (inventory.items[i].item.type == targetType) {
      indices.push_back(i);
    }
  }
  return indices;
}

EquipRequest EquipmentMenu::requestEquip(const std::vector<int>& candidates) const {
  EquipmentSlot slot = EquipmentSlot::Weapon;
  if (selectedSlot_ == 1) slot = EquipmentSlot::Armor;
  if (selectedSlot_ == 2) slot = EquipmentSlot::Accessory;
  return {slot, candidates[cursor_]};
}

void EquipmentMenu::completeEquip() {
  state_ = State::SLOT;
}

EquipmentMenuResult EquipmentMenu::cancel() {
  if (state_ == State::CANDIDATE) {
    state_ = State::SLOT;
    return EquipmentMenuResult::NONE;
  }
  return EquipmentMenuResult::BACK;
}

EquipmentMenu::State EquipmentMenu::state() const {
  return state_;
}

int EquipmentMenu::cursor() const {
  return cursor_;
}

int EquipmentMenu::selectedSlot() const {
  return selectedSlot_;
}
