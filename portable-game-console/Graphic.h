#pragma once

#include "LGFX_RP2040_ILI9341.hpp"
#include "Player.h"

class Map;

class Graphic {
public:
  static const int SCREEN_WIDTH = 320;
  static const int SCREEN_HEIGHT = 240;
  
  // ★ ラインバッファの高さ (32px)
  static const int BLOCK_HEIGHT = 32; 
  
  LGFX lcd;
  LGFX_Sprite lineSprite; // ★ 画面全体ではなく、帯状の小さなスプライト

  // カメラ座標
  int cameraX = 0;
  int cameraY = 0;

  // 前フレームのヒーロー矩形（画面座標）
  int prevHeroScreenX = -1;
  int prevHeroScreenY = -1;
  Player::Direction prevHeroDirection = Player::DIR_DOWN;
  
  // 強制再描画フラグ
  int startTileX = -1; 
  // (以下の変数は関数内で計算するのでメンバ変数として保持する必要はないが、互換性のため残すなら残す)
  int startTileY = -1;
  int visTileW = 0; 
  int visTileH = 0;

  int currentFloorNumber = 1;
  void setFloor(int floor);
  // コンストラクタで lineSprite を初期化
  Graphic();

  void init();

  // tileType からビットマップポインタを返す小ヘルパ
  const uint16_t* tileBitmapFor(int tileType);

  const uint16_t* getHeroImage(int dir);

  // ★ ラインバッファを使って背景全体を描画 (hero を引数に追加)
  void drawFullBackground(Map &map, const Player &hero, int cameraX_in, int cameraY_in);

  // 指定した矩形範囲(32x32)の背景をLCDに直接復元するヘルパー
  // (restorePrevHeroArea の代わり)
  void restoreRectDirect(Map &map, int sx, int sy);

  // smartRender (部分更新)
  // ヒーローが動いた時だけ、ヒーロー周辺だけを描き直す
  void smartRender(Map &map, const Player &hero, int desiredCameraX, int desiredCameraY);

  // 外部から強制再描画を要求するときに呼ぶ
  void invalidateAndRedraw(Map &map, const Player &hero, int desiredCameraX, int desiredCameraY);
};
