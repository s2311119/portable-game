#include "Player.h"

#include "Map.h"
#include "CharacterState.h"

Player::Player(Map& map, int level, int who) {
    status = new CharacterState(level, who);
    bool positionFound = false;
    // 最初の草タイル(TILE_GRASS)を探す
    for (int y = 1; y < map.getMAP_HEIGHT() - 1; y++) {
      for (int x = 1; x < map.getMAP_WIDTH() - 1; x++) {
        if (map.getTile(x, y) == Map::TILE_GRASS) {
          // 見つけたら、そこをピクセル座標のスタート地点に
          X = x * Map::TILE_SIZE;
          Y = y * Map::TILE_SIZE;
          oldX = X;
          oldY = Y;
          positionFound = true;
          break;
        }
      }
      if (positionFound) break;
      direction = DIR_DOWN;
    }
    
    // もし万が一、草タイルが一つも見つからなかった場合の保険
    if (!positionFound) { 
      X = Map::TILE_SIZE; Y = Map::TILE_SIZE; oldX = X; oldY = Y;
    }
}

void Player::reset(Map& map) {
    // Mapが決めた開始位置を使用
    Map::ObjectPos startPos =
        map.getPlayerStartPos();

    X = startPos.x * Map::TILE_SIZE;
    Y = startPos.y * Map::TILE_SIZE;

    oldX = X;
    oldY = Y;
}

Player::~Player() {
    if (status != nullptr) {
      delete status;
      status = nullptr;
    }
}
