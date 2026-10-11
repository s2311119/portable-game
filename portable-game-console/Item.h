#pragma once

class Item {
public:
  enum ItemType {
    ITEM_CONSUMABLE, // 消費アイテム (回復薬など)
    ITEM_WEAPON,     // 武器
    ITEM_ARMOR,      // 防具
    ITEM_ACCESSORY,  // アクセサリー
    ITEM_MATERIAL,   // 素材 (換金用)
    ITEM_IMPORTANT   // 大事なもの
  };

  ItemType type;
  int value;
  int price;
  int id;
  const char* name;

  Item(
      int _id = 0,
      const char* _name = "",
      ItemType _type = ITEM_MATERIAL,
      int _value = 0,
      int _price = 0)
  {
    id = _id;
    name = _name;
    type = _type;
    value = _value;
    price = _price;
  }
};