#include "GameSession.h"

GameSession::GameSession()
    : progressValue(),
      mapValue(20, 20, Map::TYPE_TOWN, progressValue.floor()),
      playerValue(mapValue, 1, 38),
      inventoryValue(),
      partyValue(),
      storageValue() {}

WorldProgress& GameSession::progress() { return progressValue; }
const WorldProgress& GameSession::progress() const { return progressValue; }
Map& GameSession::map() { return mapValue; }
const Map& GameSession::map() const { return mapValue; }
Player& GameSession::player() { return playerValue; }
const Player& GameSession::player() const { return playerValue; }
Inventory& GameSession::inventory() { return inventoryValue; }
const Inventory& GameSession::inventory() const { return inventoryValue; }
Party& GameSession::party() { return partyValue; }
const Party& GameSession::party() const { return partyValue; }
AutomatonStorage& GameSession::storage() { return storageValue; }
const AutomatonStorage& GameSession::storage() const { return storageValue; }
