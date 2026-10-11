#pragma once

#include "Map.h"

class Sd;
class MUSIC;
class WorldProgress;
class Player;
class Party;
class AutomatonStorage;
class Inventory;
class GameSession;

struct LoadGameResult {
  bool success;
  bool requiresMapRegeneration;
  Map::MapType legacyMapType;
};

class SaveManager {
public:
  const char* SAVE_FILENAME = "/save.dat";
  const char* SAVE_MAGIC = "PGSAVE2";

  bool saveGame(Sd& sd, MUSIC& music, const GameSession& session);

  LoadGameResult loadGame(Sd& sd, MUSIC& music, GameSession& session);

  // Menuからの一時的なmigration bridge。MenuFlow移行時に除去する。
  bool saveGame(
      Sd& sd,
      MUSIC& music,
      Map& map,
      WorldProgress& progress,
      Player& hero,
      Party& party,
      AutomatonStorage& storage,
      Inventory& inventory
  );

  bool hasSaveFile(Sd& sd);

private:
  bool saveComponents(
      Sd& sd,
      MUSIC& music,
      const Map& map,
      const WorldProgress& progress,
      const Player& hero,
      const Party& party,
      const AutomatonStorage& storage,
      const Inventory& inventory
  );
};
