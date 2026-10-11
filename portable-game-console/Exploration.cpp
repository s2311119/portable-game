#include "Exploration.h"

#include "Map.h"
#include "Player.h"

void Exploration::updateMovement(
    Player& hero,
    Map& map,
    uint32_t nowMs,
    const ExplorationInput& input) {
  move(hero, nowMs, input);
  if (input.isPressed(FieldDirection::UP))    hero.direction = Player::DIR_UP;
  if (input.isPressed(FieldDirection::DOWN))  hero.direction = Player::DIR_DOWN;
  if (input.isPressed(FieldDirection::LEFT))  hero.direction = Player::DIR_LEFT;
  if (input.isPressed(FieldDirection::RIGHT)) hero.direction = Player::DIR_RIGHT;
  checkWall(hero, map);
}

void Exploration::move(Player& c, uint32_t nowMs, const ExplorationInput& input) {
    uint32_t now = nowMs;
    if (now - lastMoveTime < moveIntervalMs) return; // まだ移動周期に達していない
    // 1ステップだけ進める（長押しで連続移動だが、moveIntervalMsで抑止）
    if (input.isPressed(FieldDirection::UP))    c.Y -= c.MOVE_SPEED;
    if (input.isPressed(FieldDirection::DOWN))  c.Y += c.MOVE_SPEED;
    if (input.isPressed(FieldDirection::LEFT))  c.X -= c.MOVE_SPEED;
    if (input.isPressed(FieldDirection::RIGHT)) c.X += c.MOVE_SPEED;
    lastMoveTime = now;
  }

void Exploration::checkWall(Player &hero, Map &map) {
    int hitbox_offset = 1;

    int centerX =
        hero.X + Map::TILE_SIZE / 2;

    int centerY =
        hero.Y + Map::TILE_SIZE / 2;

    int left   = centerX - hitbox_offset;
    int right  = centerX + hitbox_offset;
    int top    = centerY - hitbox_offset;
    int bottom = centerY + hitbox_offset;

    const int mapPixelWidth =
        map.getMAP_WIDTH() * Map::TILE_SIZE;

    const int mapPixelHeight =
        map.getMAP_HEIGHT() * Map::TILE_SIZE;

    // マップ外は壁として扱う
    auto isWallOrOutOfBounds =
        [&](int px, int py) -> bool
    {
      if (px < 0 ||
          py < 0 ||
          px >= mapPixelWidth ||
          py >= mapPixelHeight)
      {
        return true;
      }

      int tileX =
          px / Map::TILE_SIZE;

      int tileY =
          py / Map::TILE_SIZE;

      return map.getTile(tileX, tileY) == Map::TILE_WALL;
    };

    if (isWallOrOutOfBounds(left,  top) ||
        isWallOrOutOfBounds(right, top) ||
        isWallOrOutOfBounds(left,  bottom) ||
        isWallOrOutOfBounds(right, bottom))
    {
      hero.X = hero.oldX;
      hero.Y = hero.oldY;
      hero.moved = false;
    }
  }
