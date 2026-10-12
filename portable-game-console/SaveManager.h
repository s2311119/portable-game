#pragma once

#include "Map.h"

class Sd;
class AudioManager;
class WorldProgress;
class Player;
class Party;
class AutomatonStorage;
class Inventory;
class GameSession;
class Settings;

struct LoadGameResult {
  bool success;
  bool requiresMapRegeneration;
  Map::MapType legacyMapType;
};

class SaveManager {
public:
  const char* SAVE_FILENAME = "/save.dat";
  const char* SAVE_MAGIC = "PGSAVE2";

  bool saveGame(Sd& sd, AudioManager& music, const GameSession& session);

  LoadGameResult loadGame(Sd& sd, AudioManager& music, GameSession& session);

  bool saveSettings(Sd& sd, AudioManager& music, const Settings& settings);
  bool loadSettings(Sd& sd, Settings& settings);

  bool hasSaveFile(Sd& sd);

private:
  const char* SETTINGS_FILENAME = "/settings.dat";
  const char* SETTINGS_MAGIC = "PGSET1";

  bool saveComponents(
      Sd& sd,
      AudioManager& music,
      const Map& map,
      const WorldProgress& progress,
      const Player& hero,
      const Party& party,
      const AutomatonStorage& storage,
      const Inventory& inventory
  );
};
