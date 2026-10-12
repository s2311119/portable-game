#include "ItemUseSystem.h"

#include "Inventory.h"
#include "Player.h"
#include "Party.h"

bool ItemUseSystem::canUse(int itemId) const {
  return itemId >= 100 && itemId < 200;
}

void ItemUseSystem::use(const ItemUseRequest& request, Inventory& inventory,
                        Player& player, Party& party) const {
  int itemId = inventory.items[request.itemIndex].item.id;
  CharacterState* targetStatus = nullptr;
  if (request.targetIndex == 0) targetStatus = &player.state();
  else targetStatus = &party.memberState(request.targetIndex - 1);

  if (itemId == 100) targetStatus->heal(50);
  else if (itemId == 101) targetStatus->heal(200);
  else if (itemId == 102) targetStatus->heal(9999);
  else if (itemId == 103) targetStatus->restoreTP(20);
  else if (itemId == 104) targetStatus->CureStatus(true);

  inventory.removeByIndex(request.itemIndex);
}
