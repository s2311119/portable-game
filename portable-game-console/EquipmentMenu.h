#pragma once

#include <vector>
#include "Equipment.h"

class Inventory;

struct EquipRequest {
  EquipmentSlot slot;
  int inventoryIndex;
};

enum class EquipmentMenuResult {
  NONE,
  BACK
};

class EquipmentMenu {
public:
  enum class State {
    SLOT,
    CANDIDATE
  };

private:
  State state_ = State::SLOT;
  int cursor_ = 0;
  int selectedSlot_ = 0;

public:
  void open();
  void moveUp(int count);
  void moveDown(int count);
  void selectSlot();
  std::vector<int> candidateIndices(const Inventory& inventory) const;
  EquipRequest requestEquip(const std::vector<int>& candidates) const;
  void completeEquip();
  EquipmentMenuResult cancel();

  State state() const;
  int cursor() const;
  int selectedSlot() const;
};
