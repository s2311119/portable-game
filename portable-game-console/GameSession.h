#pragma once

#include "WorldProgress.h"
#include "Map.h"
#include "Player.h"
#include "Inventory.h"
#include "Party.h"
#include "AutomatonStorage.h"

class GameSession {
public:
  GameSession();

  WorldProgress& progress();
  const WorldProgress& progress() const;
  Map& map();
  const Map& map() const;
  Player& player();
  const Player& player() const;
  Inventory& inventory();
  const Inventory& inventory() const;
  Party& party();
  const Party& party() const;
  AutomatonStorage& storage();
  const AutomatonStorage& storage() const;

private:
  WorldProgress progressValue;
  Map mapValue;
  Player playerValue;
  Inventory inventoryValue;
  Party partyValue;
  AutomatonStorage storageValue;
};
