#pragma once

#include "Equipment.h"

class Inventory;
class CharacterState;

class EquipmentChangeSystem {
public:
  static void equip(EquipmentSlot slot, int inventoryIndex,
                    Inventory& inventory, CharacterState& character);
};
