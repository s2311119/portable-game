#pragma once

enum class EquipmentSlot {
  Weapon,
  Armor,
  Accessory
};

class Equipment {
public:
  void equip(EquipmentSlot slot, int itemId);
  void unequip(EquipmentSlot slot);

  int weaponId() const;
  int armorId() const;
  int accessoryId() const;

  float applyAttackBonus(float finalValue) const;
  float applyDefenseBonus(float finalValue) const;
  float applySpeedBonus(float finalValue) const;
  float applySenseBonus(float finalValue) const;
  float applyLuckBonus(float finalValue) const;

private:
  int weaponIdValue = -1;
  int armorIdValue = -1;
  int accessoryIdValue = -1;
};
