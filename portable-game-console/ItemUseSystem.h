#pragma once

#include "ItemUseRequest.h"

class Inventory;
class Player;
class Party;

class ItemUseSystem {
public:
  bool canUse(int itemId) const;
  void use(const ItemUseRequest& request, Inventory& inventory,
           Player& player, Party& party) const;
};
