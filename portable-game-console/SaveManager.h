#pragma once

class Sd;
class MUSIC;
class Map;
class WorldProgress;
class Player;
class Party;
class AutomatonStorage;
class Inventory;

class SaveManager {
public:
  const char* SAVE_FILENAME = "/save.dat";
  const char* SAVE_MAGIC = "PGSAVE2";

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

  bool loadGame(
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
};
