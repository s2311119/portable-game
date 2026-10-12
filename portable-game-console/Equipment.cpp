#include "Equipment.h"
#include "ItemCatalog.h"

void Equipment::equip(EquipmentSlot slot, int itemId) {
  switch (slot) {
    case EquipmentSlot::Weapon: weaponIdValue = itemId; break;
    case EquipmentSlot::Armor: armorIdValue = itemId; break;
    case EquipmentSlot::Accessory: accessoryIdValue = itemId; break;
  }
}

void Equipment::unequip(EquipmentSlot slot) {
  equip(slot, -1);
}

int Equipment::weaponId() const { return weaponIdValue; }

int Equipment::armorId() const { return armorIdValue; }

int Equipment::accessoryId() const { return accessoryIdValue; }

float Equipment::applyAttackBonus(float finalValue) const {
    // 武器の補正
    if (weaponIdValue != -1) {
      Item weapon = ItemCatalog::get(weaponIdValue);
      finalValue += weapon.value; // 武器の攻撃力を足す
    }
    // アクセの補正 (例: パワーリング ID 500)
    if (accessoryIdValue == 500) finalValue += 10;
    if (accessoryIdValue == 599) finalValue += 500;
  return finalValue;
}

float Equipment::applyDefenseBonus(float finalValue) const {
    // 防具の補正
    if (armorIdValue != -1) {
      Item armor = ItemCatalog::get(armorIdValue);
      finalValue += armor.value;
    }
    if (accessoryIdValue == 501) finalValue += 10;  // ガードリング
    if (accessoryIdValue == 599) finalValue += 500; // トリニティ・コア
  return finalValue;
}

float Equipment::applySpeedBonus(float finalValue) const {
    // アクセ補正
    if (accessoryIdValue == 502) finalValue += 10;  // クイックブーツ
    if (accessoryIdValue == 599) finalValue += 500; // トリニティ・コア
  return finalValue;
}

float Equipment::applySenseBonus(float finalValue) const {
    // アクセ補正
    if (accessoryIdValue == 503) finalValue += 10;  // スナイパーレンズ
    if (accessoryIdValue == 599) finalValue += 500; // トリニティ・コア
  return finalValue;
}

float Equipment::applyLuckBonus(float finalValue) const {
    // アクセ補正
    if (accessoryIdValue == 504) finalValue += 50;  // ラッキーコイン
    if (accessoryIdValue == 599) finalValue += 500; // トリニティ・コア

  return finalValue;
}
