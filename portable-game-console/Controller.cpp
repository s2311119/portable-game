#include "Controller.h"
#include <Arduino.h>
#include "Caractor.h"
#include "Map.h"

Controller::Controller() {
    for (int i = 0; i < 8; ++i) {
      pinMode(btnPins[i], INPUT_PULLUP);
      lastPressTime[i] = 0;
      // 押下状態（LOW）をboolで保持
      prevState[i] = (digitalRead(btnPins[i]) == LOW);
    }
  }

bool Controller::pressed(int pin) {
    return digitalRead(pin) == LOW;
  }

bool Controller::pressedDebounced(int pin) {
    // find index
    int idx = -1;
    for (int i = 0; i < 8; ++i) if (btnPins[i] == pin) { idx = i; break; }
    if (idx < 0) return false; // unknown pin

    bool isPressed = (digitalRead(pin) == LOW);
    if (isPressed) {
      uint32_t now = millis();
      if (now - lastPressTime[idx] > debounceMs) {
        lastPressTime[idx] = now;
        return true;
      }
    }
    return false;
  }

void Controller::update_cara_point(Caractor &c) {
    uint32_t now = millis();
    if (now - lastMoveTime < moveIntervalMs) return; // まだ移動周期に達していない
    // 1ステップだけ進める（長押しで連続移動だが、moveIntervalMsで抑止）
    if (pressed(BTN_UP))    c.Y -= c.MOVE_SPEED;
    if (pressed(BTN_DOWN))  c.Y += c.MOVE_SPEED;
    if (pressed(BTN_LEFT))  c.X -= c.MOVE_SPEED;
    if (pressed(BTN_RIGHT)) c.X += c.MOVE_SPEED;
    lastMoveTime = now;
  }

void Controller::check_wall(Caractor &hero, Map &map) {
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

bool Controller::check_event(Caractor &hero, Map &map, GameState &state, EventType &currentEvent) {
    // 1. プレイヤーの現在タイル座標を取得
    int heroTileX =
        (hero.X + Map::TILE_SIZE / 2) /
        Map::TILE_SIZE;

    int heroTileY =
        (hero.Y + Map::TILE_SIZE / 2) /
        Map::TILE_SIZE;

    // 万一マップ外ならイベント判定をしない
    if (heroTileX < 0 ||
        heroTileY < 0 ||
        heroTileX >= map.getMAP_WIDTH() ||
        heroTileY >= map.getMAP_HEIGHT())
    {
      return false;
    }

    // 2. その場所のタイルタイプを取得
    uint8_t tileType = map.getTile(heroTileX, heroTileY);

    // 3. タイルタイプに応じて分岐
    switch (tileType) {
      case Map::TILE_BOX_CLOSE:
        // 閉じた宝箱の上にいる
        
        state = STATE_EVENT; // ★ プレイヤーの操作を止める
        currentEvent = EVENT_TREASURE_BOX; // ★ イベントタイプをセット
        
        return true; // イベント発生

      case Map::TILE_STAIR_UP:
        state = STATE_EVENT;
        currentEvent = EVENT_STAIR_UP;
        return true;

      case Map::TILE_STAIR_DOWN:
        state = STATE_EVENT;
        currentEvent = EVENT_STAIR_DOWN;
        return true;
      case Map::TILE_AUTOMATON:
        state = STATE_EVENT;
        currentEvent = EVENT_AUTOMATON;
        return true;
      case Map::TILE_INN:
        state = STATE_EVENT; currentEvent = EVENT_INN; return true;
      case Map::TILE_SHOP:
        state = STATE_EVENT; currentEvent = EVENT_SHOP; return true;
      case Map::TILE_STORAGE:
        state = STATE_EVENT; currentEvent = EVENT_STORAGE; return true;
      case Map::TILE_BOSS: // TILE_BOSS (Mapクラスで定義していない場合は99)
        state = STATE_EVENT;
        currentEvent = EVENT_BOSS_BATTLE;
        return true;
      default:
        // TILE_GRASS など
        return false; // イベントなし
    }
  }
