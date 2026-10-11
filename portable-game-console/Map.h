#pragma once

#include <stdint.h>
#include <vector>

class Map {
public:
  // マップの寸法（コンストラクタで決定）  
  static const int TILE_SIZE = 32;
  static const int TILE_GRASS = 0;
  static const int TILE_WALL  = 1;
  static const int TILE_STAIR_UP = 2;
  static const int TILE_STAIR_DOWN = 3;
  static const int TILE_BOX_CLOSE = 4;
  static const int TILE_BOX_OPEN = 5;
  static const int TILE_AUTOMATON = 6;
  static const int TILE_INN = 7;     // 宿屋
  static const int TILE_SHOP = 8;    // 店
  static const int TILE_STORAGE = 9; // 預かり所
  static const int TILE_BOSS = 99;//ボス

  enum MapType {
    TYPE_MAZE,
    TYPE_DUNGEON,
    TYPE_TOWN,
    TYPE_BOSS_ROOM
  };

  // 部屋の矩形情報
  struct Room {
    int x, y, w, h;
  };

  struct ObjectPos {
    int x, y;
  };

  Map(int max_w, int max_h, MapType type, int floor);

  int getMAP_WIDTH();
  int getMAP_HEIGHT();

  uint8_t getTile(int x, int y) const;

  void setTile(int x, int y, uint8_t tile);

  MapType getType() const;

  void beginRestoreFromSave(
      MapType type,
      int width,
      int height);

  bool isTown() const;

  bool isBossRoom() const;

  ObjectPos getPlayerStartPos() const;

  int getAutomatonWhoId() const;

  void restoreAutomatonWhoId(int whoId);

  void regenerate(
      int max_w,
      int max_h,
      MapType type,
      int floor);
private:
  MapType currentType;

  int MAP_WIDTH;
  int MAP_HEIGHT;
  std::vector<uint8_t> mapData;

  std::vector<ObjectPos> boxPositions;
  ObjectPos stairUpPos = {-1, -1};
  ObjectPos stairDownPos = {-1, -1};
  ObjectPos playerStartPos;
  int automatonWhoId = -1;
  void generateMapContent(MapType type, int floor);

  void setMapDimensions(int max_w, int max_h, MapType type);

  void generateBossRoom();

  std::vector<int> pathX, pathY;
  std::vector<int> dirs;
  std::vector<Room> placedRooms;
  std::vector<ObjectPos> grassTiles;
  void generateMazeAndRooms(MapType type, int floor);

  // (generateDungeon のヘルパー関数) 部屋を掘る
  void carveRoom(int x1, int y1, int x2, int y2);

  void placeObjects(int floor);
};
