#include "Map.h"

#include <Arduino.h>
#include <algorithm>

Map::Map(int max_w, int max_h, MapType type, int floor)
  {
    currentType = type;

    setMapDimensions(
        max_w,
        max_h,
        type
    );

    generateMapContent(type, floor);
  }

int Map::getMAP_WIDTH() const {
    return MAP_WIDTH;
  }

int Map::getMAP_HEIGHT() const {
    return MAP_HEIGHT;
  }

uint8_t Map::getTile(int x, int y) const {
    return mapData[y * MAP_WIDTH + x];
  }

void Map::setTile(int x, int y, uint8_t tile) {
    mapData[y * MAP_WIDTH + x] = tile;
  }

Map::MapType Map::getType() const {
    return currentType;
  }

void Map::beginRestoreFromSave(
      MapType type,
      int width,
      int height)
  {
    currentType = type;
    MAP_WIDTH = width;
    MAP_HEIGHT = height;

    mapData.assign(
        MAP_WIDTH * MAP_HEIGHT,
        TILE_WALL
    );
  }

bool Map::isTown() const {
    return currentType == TYPE_TOWN;
  }

bool Map::isBossRoom() const {
    return currentType == TYPE_BOSS_ROOM;
  }

Map::ObjectPos Map::getPlayerStartPos() const {
    return playerStartPos;
  }

int Map::getAutomatonWhoId() const {
    return automatonWhoId;
  }

void Map::restoreAutomatonWhoId(int whoId) {
    automatonWhoId = whoId;
  }

void Map::regenerate(
      int max_w,
      int max_h,
      MapType type,
      int floor)
  {
    currentType = type;

    setMapDimensions(
        max_w,
        max_h,
        type
    );

    generateMapContent(type, floor);
  }

void Map::generateMapContent(MapType type, int floor) {
    mapData.assign(
        MAP_WIDTH * MAP_HEIGHT,
        TILE_WALL
    );

    if (type == TYPE_BOSS_ROOM) {
      generateBossRoom();
      return;
    }

    generateMazeAndRooms(type, floor);
    placeObjects(floor);
  }

void Map::setMapDimensions(int max_w, int max_h, MapType type) {
    if (type == TYPE_TOWN) {
      MAP_WIDTH = 8;
      MAP_HEIGHT = 8;
    }
    else if (type == TYPE_BOSS_ROOM) {
      MAP_WIDTH = 9;
      MAP_HEIGHT = 9;
    }
    else {
      const int MIN_DUNGEON_SIZE = 15;

      int safeMaxW =
          (max_w < MIN_DUNGEON_SIZE)
              ? MIN_DUNGEON_SIZE
              : max_w;

      int safeMaxH =
          (max_h < MIN_DUNGEON_SIZE)
              ? MIN_DUNGEON_SIZE
              : max_h;

      int maxOddW =
          (safeMaxW % 2 == 0)
              ? safeMaxW - 1
              : safeMaxW;

      int maxOddH =
          (safeMaxH % 2 == 0)
              ? safeMaxH - 1
              : safeMaxH;

      MAP_WIDTH =
          random(
              (MIN_DUNGEON_SIZE - 1) / 2,
              (maxOddW - 1) / 2 + 1
          ) * 2 + 1;

      MAP_HEIGHT =
          random(
              (MIN_DUNGEON_SIZE - 1) / 2,
              (maxOddH - 1) / 2 + 1
          ) * 2 + 1;
    }
  }

void Map::generateBossRoom() {
    // 全体を床にする
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
      mapData[i] = TILE_GRASS;
    }

    // 外周を壁にする
    for (int x = 0; x < MAP_WIDTH; x++) {
      mapData[x] = TILE_WALL;
      mapData[(MAP_HEIGHT - 1) * MAP_WIDTH + x] = TILE_WALL;
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
      mapData[y * MAP_WIDTH] = TILE_WALL;
      mapData[y * MAP_WIDTH + (MAP_WIDTH - 1)] = TILE_WALL;
    }

    // 入り口となる下り階段
    int stairX = MAP_WIDTH / 2;
    int stairY = MAP_HEIGHT - 3;

    mapData[stairY * MAP_WIDTH + stairX] =
        TILE_STAIR_DOWN;

    playerStartPos = {stairX, stairY};

    // ボスを部屋の奥に配置
    int bossX = MAP_WIDTH / 2;
    int bossY = 5;

    mapData[bossY * MAP_WIDTH + bossX] =
        TILE_BOSS;
  }

void Map::generateMazeAndRooms(MapType type, int floor) {
    if (type == TYPE_TOWN) {

      // 2. 中央に大きな広場(草)を作る
      int margin = 2; 
      // マップが小さすぎる場合の保護
      if (MAP_WIDTH <= margin * 2 || MAP_HEIGHT <= margin * 2) margin = 1;

      for(int y = margin; y < MAP_HEIGHT - margin; y++) {
        for(int x = margin; x < MAP_WIDTH - margin; x++) {
          mapData[y * MAP_WIDTH + x] = TILE_GRASS;
        }
      }
      // 集落生成はこれで終わり（階段などは placeObjects で）
      return; 
    }
    if (floor >= 21 && random(100) < 5) { // 例: 5%の確率
      automatonWhoId = 75;
    } 
    else {
      // 通常の仲間
      const int basicAllies[] = {39, 40, 41, 42, 43};
      automatonWhoId = basicAllies[random(5)];
    }
    // --- 1. 穴掘り法 ---
    int startX = 1, startY = 1;
    
    // ★ メンバー変数をクリアして再利用 (new しない)
    pathX.clear(); pathY.clear();
    pathX.reserve(MAP_WIDTH * MAP_HEIGHT / 4);
    pathY.reserve(MAP_WIDTH * MAP_HEIGHT / 4);
    
    pathX.push_back(startX); pathY.push_back(startY);
    mapData[startY * MAP_WIDTH + startX] = TILE_GRASS;
    int dx[]={0,2,0,-2}, dy[]={-2,0,2,0};

    while(!pathX.empty()){
      int cx=pathX.back(), cy=pathY.back();
      
      dirs.clear(); // ★ メンバー変数を再利用
      dirs.reserve(4);
      
      for(int i=0;i<4;i++){
        int nx=cx+dx[i], ny=cy+dy[i];
        if(nx>0 && nx<MAP_WIDTH-1 && ny>0 && ny<MAP_HEIGHT-1 && mapData[ny * MAP_WIDTH + nx]==TILE_WALL) dirs.push_back(i);
      }
      if(!dirs.empty()){
        int dir = dirs[random((int)dirs.size())];
        int wallX = cx + dx[dir]/2, wallY = cy + dy[dir]/2;
        int nextX = cx + dx[dir], nextY = cy + dy[dir];
        mapData[wallY * MAP_WIDTH + wallX] = TILE_GRASS;
        mapData[nextY * MAP_WIDTH + nextX] = TILE_GRASS;
        pathX.push_back(nextX); pathY.push_back(nextY);
      } else {
        pathX.pop_back(); pathY.pop_back();
      }
    }

    // --- 2. rooms ---
    if(type == TYPE_DUNGEON) {
      int numRooms = random(0, 3);
      int minRoomSize = 3;
      int maxRoomSize = 8;

      placedRooms.clear(); // ★ メンバー変数を再利用
      placedRooms.reserve(numRooms);

      int maxAttempts = 100;

      for (int i = 0; i < numRooms; i++) {
        bool roomPlaced = false;
        for (int attempt = 0; attempt < maxAttempts; attempt++) {
          int w = random(minRoomSize, maxRoomSize + 1);
          int h = random(minRoomSize, maxRoomSize + 1);
          int x = random(1, MAP_WIDTH - w - 2);
          int y = random(1, MAP_HEIGHT - h - 2);
          Room newRoom = {x, y, w, h};
          bool overlaps = false;
          for (const auto& existingRoom : placedRooms) {
            if (newRoom.x < existingRoom.x + existingRoom.w &&
                newRoom.x + newRoom.w > existingRoom.x &&
                newRoom.y < existingRoom.y + existingRoom.h &&
                newRoom.y + newRoom.h > existingRoom.y) {
              overlaps = true; break;
            }
          }
          if (!overlaps) {
            carveRoom(newRoom.x, newRoom.y, newRoom.x + newRoom.w, newRoom.y + newRoom.h);
            placedRooms.push_back(newRoom);
            roomPlaced = true;
            break;
          }
        }
      }
    }
  }

void Map::carveRoom(int x1, int y1, int x2, int y2) {
    for (int y = y1; y < y2; y++) {
      for (int x = x1; x < x2; x++) {
        if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
          mapData[y * MAP_WIDTH + x] = TILE_GRASS;
        }
      }
    }
  }

void Map::placeObjects(int floor) {
    boxPositions.clear();
    stairUpPos = {-1, -1};
    stairDownPos = {-1, -1};
    playerStartPos = {1, 1};

    grassTiles.clear(); // ★ メンバー変数を再利用
    grassTiles.reserve((MAP_WIDTH-2) * (MAP_HEIGHT-2) / 2);
    for (int y = 1; y < MAP_HEIGHT - 1; ++y) {
      for (int x = 1; x < MAP_WIDTH - 1; ++x) {
        if (mapData[y * MAP_WIDTH + x] == TILE_GRASS) grassTiles.push_back({x,y});
      }
    }
    if (grassTiles.empty()) return;
    int idx;
    
    if (currentType == TYPE_TOWN) {
      // 1. 上り階段
      idx = random((int)grassTiles.size());
      stairUpPos = grassTiles[idx];
      mapData[stairUpPos.y * MAP_WIDTH + stairUpPos.x] = TILE_STAIR_UP;
      grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();

      // 2. 下り階段 (1階以外)
      if (floor > 1 && !grassTiles.empty()) {
        idx = random((int)grassTiles.size());
        stairDownPos = grassTiles[idx];
        mapData[stairDownPos.y * MAP_WIDTH + stairDownPos.x] = TILE_STAIR_DOWN;
        grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();
      }
      if (floor > 1){
        playerStartPos = stairDownPos;
      }else{
        playerStartPos = grassTiles[random((int)grassTiles.size())];
      }
      // ★★★ 3. 施設の配置 ★★★
      if (!grassTiles.empty()) { // 宿屋
        idx = random((int)grassTiles.size());
        ObjectPos pos = grassTiles[idx];
        mapData[pos.y * MAP_WIDTH + pos.x] = TILE_INN;
        grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();
      }
      if (!grassTiles.empty()) { // 店
        idx = random((int)grassTiles.size());
        ObjectPos pos = grassTiles[idx];
        mapData[pos.y * MAP_WIDTH + pos.x] = TILE_SHOP;
        grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();
      }
      if (!grassTiles.empty()) { // 預かり所
        idx = random((int)grassTiles.size());
        ObjectPos pos = grassTiles[idx];
        mapData[pos.y * MAP_WIDTH + pos.x] = TILE_STORAGE;
        grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();
      }  
      return; 
    }

    // pick using swap-pop to avoid erase shift
    idx = random((int)grassTiles.size());
    stairDownPos = grassTiles[idx];
    playerStartPos = stairDownPos;
    mapData[stairDownPos.y * MAP_WIDTH + stairDownPos.x] = TILE_STAIR_DOWN;
    grassTiles[idx] = grassTiles.back();
    grassTiles.pop_back();

    idx = random((int)grassTiles.size());
    stairUpPos = grassTiles[idx];
    mapData[stairUpPos.y * MAP_WIDTH + stairUpPos.x] = TILE_STAIR_UP;
    grassTiles[idx] = grassTiles.back();
    grassTiles.pop_back();

    if (!grassTiles.empty()) {
      idx = random((int)grassTiles.size());
      ObjectPos allyPos = grassTiles[idx];
      // ★ TODO: 今は 39 で固定。将来的にはフロア番号で who ID を変える
      // (mapData は who ID を保持できないので、別の変数に持つ)
      mapData[allyPos.y * MAP_WIDTH + allyPos.x] = TILE_AUTOMATON; 
      grassTiles[idx] = grassTiles.back();
      grassTiles.pop_back();
    }

    int maxBoxes = std::max(0, (int)grassTiles.size() / 5);
    int numBoxes = 0;
    if (maxBoxes > 0) numBoxes = random(1, std::min(maxBoxes, 10) + 1);
    for (int i = 0; i < numBoxes; ++i) {
      if (grassTiles.empty()) break;
      int j = random((int)grassTiles.size());
      ObjectPos boxPos = grassTiles[j];
      mapData[boxPos.y * MAP_WIDTH + boxPos.x] = TILE_BOX_CLOSE;
      boxPositions.push_back(boxPos);
      grassTiles[j] = grassTiles.back();
      grassTiles.pop_back();
    }
  }
