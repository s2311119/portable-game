#include "SaveManager.h"

#include "Automaton.h"
#include "AutomatonStorage.h"
#include "GameSaveData.h"
#include "GameSession.h"
#include "Inventory.h"
#include "AudioManager.h"
#include "Party.h"
#include "Player.h"
#include "Storage.h"
#include "Settings.h"
#include "WorldProgress.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <utility>

namespace {

bool parseInteger(const String& line, int& value) {
  if (line.length() == 0) return false;
  errno = 0;
  char* end = nullptr;
  long parsed = strtol(line.c_str(), &end, 10);
  if (errno == ERANGE || end == line.c_str() || *end != '\0' ||
      parsed < INT_MIN || parsed > INT_MAX) {
    return false;
  }
  value = (int)parsed;
  return true;
}

bool readInteger(FsFile& file, int& value) {
  if (!file.available()) return false;
  String line = file.readStringUntil('\n');
  line.trim();
  return parseInteger(line, value);
}

bool readCharacter(FsFile& file, CharacterSaveData& data) {
  int skillCount = 0;
  if (!readInteger(file, data.characterId) ||
      !readInteger(file, data.level) ||
      !readInteger(file, data.xp) ||
      !readInteger(file, data.hp) ||
      !readInteger(file, data.tp) ||
      !readInteger(file, data.weaponId) ||
      !readInteger(file, data.armorId) ||
      !readInteger(file, data.accessoryId) ||
      !readInteger(file, skillCount) ||
      skillCount < 0) {
    return false;
  }
  for (int i = 0; i < skillCount; ++i) {
    int skillId = 0;
    if (!readInteger(file, skillId)) return false;
    data.skillIds.push_back(skillId);
  }
  return true;
}

bool readInventory(FsFile& file, InventorySaveData& data) {
  int count = 0;
  if (!readInteger(file, data.money) ||
      !readInteger(file, count) ||
      count < 0) {
    return false;
  }
  for (int i = 0; i < count; ++i) {
    InventoryItemSaveData item;
    if (!readInteger(file, item.itemId) ||
        !readInteger(file, item.quantity)) {
      return false;
    }
    data.items.push_back(item);
  }
  return true;
}

bool readCharacters(FsFile& file, std::vector<CharacterSaveData>& members) {
  int count = 0;
  if (!readInteger(file, count) || count < 0) return false;
  for (int i = 0; i < count; ++i) {
    CharacterSaveData member;
    if (!readCharacter(file, member)) return false;
    members.push_back(std::move(member));
  }
  return true;
}

bool readGameData(FsFile& file, GameSaveData& data, const char* magic) {
  if (!file.available()) return false;
  String firstLine = file.readStringUntil('\n');
  firstLine.trim();

  if (firstLine == magic) {
    data.format = SaveFormat::PGSAVE2;
    if (!readInteger(file, data.floor)) return false;
  } else {
    data.format = SaveFormat::LEGACY;
    if (!parseInteger(firstLine, data.floor)) return false;
  }

  int rawMapType = 0;
  if (!readInteger(file, rawMapType) ||
      rawMapType < (int)Map::TYPE_MAZE ||
      rawMapType > (int)Map::TYPE_BOSS_ROOM) {
    return false;
  }
  data.mapType = static_cast<Map::MapType>(rawMapType);

  if (data.format == SaveFormat::PGSAVE2) {
    if (!readInteger(file, data.width) ||
        !readInteger(file, data.height) ||
        data.width <= 0 || data.height <= 0 ||
        data.width > 64 || data.height > 64 ||
        !readInteger(file, data.automatonWhoId) ||
        !readInteger(file, data.playerX) ||
        !readInteger(file, data.playerY) ||
        !readInteger(file, data.playerDirection)) {
      return false;
    }

    int tileCount = 0;
    if (!readInteger(file, tileCount) ||
        tileCount != data.width * data.height) {
      return false;
    }

    for (int i = 0; i < tileCount; ++i) {
      int tile = 0;
      if (!readInteger(file, tile) || tile < 0 || tile > 255) {
        return false;
      }
      data.tiles.push_back((uint8_t)tile);
    }
  }

  if (!readCharacter(file, data.playerCharacter) ||
      !readInventory(file, data.inventory) ||
      !readCharacters(file, data.party) ||
      !readCharacters(file, data.storage)) {
    return false;
  }

  for (int i = 0; i < 5; ++i) {
    int rawFlag = 0;
    if (!readInteger(file, rawFlag)) return false;
    data.bossDefeated[i] = rawFlag != 0;
  }
  return true;
}

bool validateGameData(const GameSaveData& data) {
  if (data.mapType < Map::TYPE_MAZE ||
      data.mapType > Map::TYPE_BOSS_ROOM) {
    return false;
  }
  if (data.format == SaveFormat::LEGACY) return true;
  if (data.width <= 0 || data.height <= 0 ||
      data.width > 64 || data.height > 64 ||
      data.tiles.size() != (size_t)(data.width * data.height) ||
      data.playerDirection < (int)Player::DIR_DOWN ||
      data.playerDirection > (int)Player::DIR_RIGHT ||
      data.playerX < 0 || data.playerY < 0 ||
      data.playerX >= data.width * Map::TILE_SIZE ||
      data.playerY >= data.height * Map::TILE_SIZE) {
    return false;
  }
  return true;
}

GameSaveData captureGameData(
    const Map& map,
    const WorldProgress& progress,
    const Player& hero,
    const Party& party,
    const AutomatonStorage& storage,
    const Inventory& inventory) {
  GameSaveData data;
  data.format = SaveFormat::PGSAVE2;
  data.floor = progress.floor();
  data.mapType = map.getType();
  data.width = map.getMAP_WIDTH();
  data.height = map.getMAP_HEIGHT();
  data.automatonWhoId = map.getAutomatonWhoId();
  data.playerX = hero.X;
  data.playerY = hero.Y;
  data.playerDirection = (int)hero.direction;
  for (int y = 0; y < data.height; ++y) {
    for (int x = 0; x < data.width; ++x) {
      data.tiles.push_back(map.getTile(x, y));
    }
  }
  data.playerCharacter = hero.state().snapshotPersistentState();
  data.inventory = inventory.snapshotPersistentState();
  for (size_t i = 0; i < party.memberCount(); ++i) {
    data.party.push_back(party.member(i).state().snapshotPersistentState());
  }
  for (size_t i = 0; i < storage.memberCount(); ++i) {
    data.storage.push_back(storage.member(i).state().snapshotPersistentState());
  }
  for (int i = 0; i < 5; ++i) {
    data.bossDefeated[i] = progress.bosses().isDefeated(i);
  }
  return data;
}

void writeCharacter(FsFile& file, const CharacterSaveData& data) {
  file.println(data.characterId);
  file.println(data.level);
  file.println(data.xp);
  file.println(data.hp);
  file.println(data.tp);
  file.println(data.weaponId);
  file.println(data.armorId);
  file.println(data.accessoryId);
  file.println(data.skillIds.size());
  for (int skillId : data.skillIds) file.println(skillId);
}

void writeInventory(FsFile& file, const InventorySaveData& data) {
  file.println(data.money);
  file.println(data.items.size());
  for (const auto& item : data.items) {
    file.println(item.itemId);
    file.println(item.quantity);
  }
}

void writeGameData(FsFile& file, const GameSaveData& data, const char* magic) {
  file.println(magic);
  file.println(data.floor);
  file.println((int)data.mapType);
  file.println(data.width);
  file.println(data.height);
  file.println(data.automatonWhoId);
  file.println(data.playerX);
  file.println(data.playerY);
  file.println(data.playerDirection);
  file.println(data.width * data.height);
  for (uint8_t tile : data.tiles) file.println((int)tile);

  writeCharacter(file, data.playerCharacter);
  writeInventory(file, data.inventory);

  file.println(data.party.size());
  for (const auto& member : data.party) writeCharacter(file, member);

  file.println(data.storage.size());
  for (const auto& member : data.storage) writeCharacter(file, member);

  for (int i = 0; i < 5; ++i) file.println(data.bossDefeated[i]);
}

void applyGameData(const GameSaveData& data, GameSession& session) {
  WorldProgress& progress = session.progress();
  Map& map = session.map();
  Player& hero = session.player();
  Party& party = session.party();
  AutomatonStorage& storage = session.storage();

  progress.restoreFloor(data.floor);
  if (data.format == SaveFormat::PGSAVE2) {
    map.restoreAutomatonWhoId(data.automatonWhoId);
    map.beginRestoreFromSave(data.mapType, data.width, data.height);
    for (int y = 0; y < data.height; ++y) {
      for (int x = 0; x < data.width; ++x) {
        map.setTile(x, y, data.tiles[y * data.width + x]);
      }
    }
  }

  hero.state().restorePersistentState(data.playerCharacter);
  session.inventory().restorePersistentState(data.inventory);

  party.clearMembers();
  for (const auto& member : data.party) {
    Automaton ally(1, 0);
    ally.state().restorePersistentState(member);
    party.appendRestoredMember(std::move(ally));
  }

  storage.clearForRestore();
  for (const auto& member : data.storage) {
    Automaton ally(1, 0);
    ally.state().restorePersistentState(member);
    storage.appendRestoredMember(std::move(ally));
  }

  for (int i = 0; i < 5; ++i) {
    progress.restoreBossDefeated(i, data.bossDefeated[i]);
  }

  if (data.format == SaveFormat::PGSAVE2) {
    hero.X = data.playerX;
    hero.Y = data.playerY;
    hero.oldX = data.playerX;
    hero.oldY = data.playerY;
    hero.direction = (Player::Direction)data.playerDirection;
    hero.moved = false;
  }
}

} // namespace

bool SaveManager::saveGame(Storage& sd, AudioManager& music, const GameSession& session) {
  return saveComponents(sd, music, session.map(), session.progress(),
      session.player(), session.party(), session.storage(), session.inventory());
}

bool SaveManager::saveComponents(
    Storage& sd,
    AudioManager& music,
    const Map& map,
    const WorldProgress& progress,
    const Player& hero,
    const Party& party,
    const AutomatonStorage& storage,
    const Inventory& inventory) {
  sd.beginSynchronizedWrite(music);

  FsFile file = sd.openFile(SAVE_FILENAME, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    sd.endSynchronizedWrite(music);
    return false;
  }

  GameSaveData data = captureGameData(map, progress, hero, party, storage, inventory);
  writeGameData(file, data, SAVE_MAGIC);
  file.close();

  sd.endSynchronizedWrite(music);
  return true;
}

LoadGameResult SaveManager::loadGame(Storage& sd, AudioManager& music, GameSession& session) {
  (void)music; // load synchronization is owned by the top-level caller.
  LoadGameResult failure = {false, false, Map::TYPE_DUNGEON};
  FsFile file = sd.openFile(SAVE_FILENAME, O_READ);
  if (!file) return failure;

  GameSaveData data;
  const bool valid = readGameData(file, data, SAVE_MAGIC) && validateGameData(data);
  file.close();
  if (!valid) return failure;

  applyGameData(data, session);
  return {true, data.format == SaveFormat::LEGACY, data.mapType};
}

bool SaveManager::saveSettings(Storage& sd, AudioManager& music, const Settings& settings) {
  sd.beginSynchronizedWrite(music);

  FsFile file = sd.openFile(SETTINGS_FILENAME, O_WRITE | O_CREAT | O_TRUNC);
  if (!file) {
    sd.endSynchronizedWrite(music);
    return false;
  }

  file.println(SETTINGS_MAGIC);
  file.println(settings.getBgmVolume());
  file.println(settings.getSeVolume());
  file.println(settings.isVibrationEnabled() ? 1 : 0);
  file.close();

  sd.endSynchronizedWrite(music);
  return true;
}

bool SaveManager::loadSettings(Storage& sd, Settings& settings) {
  FsFile file = sd.openFile(SETTINGS_FILENAME, O_READ);
  if (!file) return false;

  String magic = file.readStringUntil('\n');
  magic.trim();
  int bgmVolume = 0;
  int seVolume = 0;
  int vibration = 0;
  const bool valid = magic == SETTINGS_MAGIC &&
      readInteger(file, bgmVolume) &&
      readInteger(file, seVolume) &&
      readInteger(file, vibration) &&
      bgmVolume >= 0 && bgmVolume <= 10 &&
      seVolume >= 0 && seVolume <= 10 &&
      (vibration == 0 || vibration == 1) &&
      !file.available();
  file.close();
  if (!valid) return false;

  settings.setBgmVolume(bgmVolume);
  settings.setSeVolume(seVolume);
  settings.setVibrationEnabled(vibration == 1);
  return true;
}

bool SaveManager::hasSaveFile(Storage& sd) {
  return sd.existsFile(SAVE_FILENAME);
}
