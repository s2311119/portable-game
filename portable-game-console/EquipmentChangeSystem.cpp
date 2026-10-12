#include "EquipmentChangeSystem.h"

#include "Inventory.h"
#include "CharacterState.h"
#include "ItemCatalog.h"

void EquipmentChangeSystem::equip(EquipmentSlot slot, int inventoryIndex,
                                  Inventory& inventory,
                                  CharacterState& character) {
  Item newItem = inventory.items[inventoryIndex].item;

  int oldId = -1;
  if (slot == EquipmentSlot::Weapon)
    oldId = character.equipmentState().weaponId();
  if (slot == EquipmentSlot::Armor)
    oldId = character.equipmentState().armorId();
  if (slot == EquipmentSlot::Accessory)
    oldId = character.equipmentState().accessoryId();
  if (oldId != -1) inventory.addItem(ItemCatalog::get(oldId));

  character.equip(slot, newItem.id);
  inventory.removeByIndex(inventoryIndex);
}
