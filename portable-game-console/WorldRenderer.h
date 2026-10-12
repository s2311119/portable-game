#pragma once

#include "Graphic.h"
#include "Player.h"

class Map;

class WorldRenderer {
public:
  void setFloor(int floor);
  void drawFullBackground(Graphic& graphic, const Map& map, const Player& hero,
                          int cameraX_in, int cameraY_in);
  void smartRender(Graphic& graphic, const Map& map, const Player& hero,
                   int desiredCameraX, int desiredCameraY);
  void invalidateAndRedraw(Graphic& graphic, const Map& map, const Player& hero,
                           int desiredCameraX, int desiredCameraY);
  void invalidatePreviousHero();

private:
  static const int SCREEN_WIDTH = Graphic::SCREEN_WIDTH;
  static const int SCREEN_HEIGHT = Graphic::SCREEN_HEIGHT;
  static const int BLOCK_HEIGHT = Graphic::BLOCK_HEIGHT;

  int cameraX = 0;
  int cameraY = 0;
  int prevHeroScreenX = -1;
  int prevHeroScreenY = -1;
  Player::Direction prevHeroDirection = Player::DIR_DOWN;
  int startTileX = -1;
  int startTileY = -1;
  int visTileW = 0;
  int visTileH = 0;
  int currentFloorNumber = 1;

  const uint16_t* tileBitmapFor(int tileType);
  const uint16_t* getHeroImage(int dir);
  void restoreRectDirect(Graphic& graphic, const Map& map, int sx, int sy);
};
