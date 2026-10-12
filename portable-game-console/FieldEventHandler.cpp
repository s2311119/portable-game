#include "FieldEventHandler.h"

#include "Automaton.h"
#include "AutomatonStorage.h"
#include "Inventory.h"
#include "ItemCatalog.h"
#include "Map.h"
#include "Party.h"

#include <utility>

FieldEventResult FieldEventHandler::handle(
    const FieldEvent& event,
    Map& map,
    Inventory& inventory,
    Party& party,
    AutomatonStorage& storage,
    int floor,
    const FieldRandom& random) const {
  switch (event.type) {
    case FieldEventType::TREASURE:
      return handleTreasure(event, map, inventory, floor, random);
    case FieldEventType::AUTOMATON:
      return handleAutomaton(event, map, party, storage);
    case FieldEventType::STAIR_UP:
      return {FieldEventAction::FLOOR_UP};
    case FieldEventType::STAIR_DOWN:
      return {FieldEventAction::FLOOR_DOWN};
    case FieldEventType::INN:
      return {FieldEventAction::OPEN_INN};
    case FieldEventType::SHOP:
      return {FieldEventAction::OPEN_SHOP};
    case FieldEventType::STORAGE:
      return {FieldEventAction::OPEN_STORAGE};
    case FieldEventType::BOSS_BATTLE:
      return {FieldEventAction::START_BOSS_BATTLE};
    case FieldEventType::NONE:
      return {FieldEventAction::NONE};
  }
  return {FieldEventAction::NONE};
}

FieldEventResult FieldEventHandler::handleTreasure(
    const FieldEvent& event,
    Map& map,
    Inventory& inventory,
    int floor,
    const FieldRandom& random) const {
  map.setTile(event.tileX, event.tileY, Map::TILE_BOX_OPEN);

  int itemId = 0;
  int r = random.drawMax(100);

  if (r < 45) {
    if (floor <= 10) itemId = 100;
    else if (floor <= 30) itemId = 103;
    else itemId = 101;
  }
  else if (r < 75) {
    itemId = 2;
  }
  else if (r < 90) {
    if (floor <= 20) {
      itemId = 1;
    }
    else if (floor <= 30) {
      itemId = 3 + random.drawRange(0, 2);
    }
    else {
      itemId = 5;
    }
  }
  else if (r < 93) {
    int minSkill = 1, maxSkill = 5;
    if (floor > 10) { minSkill = 6; maxSkill = 11; }
    if (floor > 30) { minSkill = 12; maxSkill = 13; }
    itemId = 200 + random.drawRange(minSkill, maxSkill + 1);
  }
  else {
    int tier = (floor - 1) / 10;
    if (tier > 4) tier = 4;

    int typeR = random.drawRange(0, 3);
    bool isRare = (random.drawMax(100) < 10);

    if (typeR == 0) {
      if (isRare && floor >= 30) itemId = 399;
      else itemId = 300 + tier;
    }
    else if (typeR == 1) {
      if (isRare && floor >= 30) itemId = 499;
      else itemId = 400 + tier;
    }
    else {
      if (isRare && floor >= 30) itemId = 599;
      else itemId = 500 + random.drawRange(0, 5);
    }
  }

  Item foundItem = ItemCatalog::get(itemId);
  inventory.addItem(foundItem);
  return {FieldEventAction::RETURN_TO_FIELD};
}

FieldEventResult FieldEventHandler::handleAutomaton(
    const FieldEvent& event,
    Map& map,
    Party& party,
    AutomatonStorage& storage) const {
  int who_to_add = map.getAutomatonWhoId();
  Automaton* newAlly = new Automaton(1, who_to_add);

  int result = 0;
  if (party.addMember(std::move(*newAlly))) {
    delete newAlly;
    result = 1;
  } else {
    storage.store(std::move(*newAlly));
    delete newAlly;
    result = 2;
  }

  if (result == 1) {
    // パーティーに入った
  } else if (result == 2) {
    // 預かり所へ送られた (自動)
  }

  map.setTile(event.tileX, event.tileY, Map::TILE_GRASS);
  return {FieldEventAction::RETURN_TO_FIELD};
}
