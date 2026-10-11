#include "Graphic.h"

#include <LGFX_TFT_eSPI.hpp>
#include "Map.h"
#include "bmp.h"

#define H_TRANSPARENT 0xFFFF

void Graphic::setFloor(int floor) {
    currentFloorNumber = floor;
  }

Graphic::Graphic() : lineSprite(&lcd) {}

void Graphic::init(){
    lcd.init();
    lcd.setRotation(1);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    
    // ★ メモリ消費はたったの 20KB！ (320x32)
    lineSprite.setColorDepth(16);
    lineSprite.setSwapBytes(true);
    lineSprite.createSprite(SCREEN_WIDTH, BLOCK_HEIGHT);
    lineSprite.setFont(&fonts::lgfxJapanGothic_12);

    lcd.setFont(&fonts::lgfxJapanGothic_12);
    //lcd.fillScreen(TFT_BLACK); // 初期化時は黒埋め
  }

const uint16_t* Graphic::tileBitmapFor(int tileType){
    switch(tileType){
      case Map::TILE_WALL: return (const uint16_t*)wall;
      case Map::TILE_STAIR_UP: return (const uint16_t*)stair_up;
      case Map::TILE_STAIR_DOWN: return (const uint16_t*)stair_down;
      case Map::TILE_BOX_CLOSE: return (const uint16_t*)box_close;
      case Map::TILE_BOX_OPEN: return (const uint16_t*)box_open;
      case Map::TILE_AUTOMATON: return (const uint16_t*)automaton_tile;
      case Map::TILE_INN: return (const uint16_t*)inn_tile;
      case Map::TILE_SHOP: return (const uint16_t*)shop_tile;
      case Map::TILE_STORAGE: return (const uint16_t*)storage_tile;
      case Map::TILE_BOSS:
        if (currentFloorNumber == 10) return (const uint16_t*)boss_img_10;
        if (currentFloorNumber == 20) return (const uint16_t*)boss_img_20;
        if (currentFloorNumber == 30) return (const uint16_t*)boss_img_30;
        if (currentFloorNumber == 40) return (const uint16_t*)boss_img_40;
        if (currentFloorNumber == 50) return (const uint16_t*)boss_img_50;
      case Map::TILE_GRASS:
      default:
        // ★★★ 階層による分岐 ★★★
        if (currentFloorNumber <= 10) return (const uint16_t*)grass_spring;
        if (currentFloorNumber <= 20) return (const uint16_t*)grass_summer;
        if (currentFloorNumber <= 30) return (const uint16_t*)grass_autumn;
        if (currentFloorNumber <= 40) return (const uint16_t*)grass_winter;
        return (const uint16_t*)grass_crazy;
    }
  }

const uint16_t* Graphic::getHeroImage(int dir) {
    switch(dir) {
      case Caractor::DIR_UP:    return (const uint16_t*)hero_up;
      case Caractor::DIR_LEFT:  return (const uint16_t*)hero_left;
      case Caractor::DIR_RIGHT: return (const uint16_t*)hero_right;
      case Caractor::DIR_DOWN:  
      default:                  return (const uint16_t*)hero_down;
    }
  }

void Graphic::drawFullBackground(Map &map, const Caractor &hero, int cameraX_in, int cameraY_in){
    // カメラ位置計算
    int maxCameraX = map.getMAP_WIDTH() * Map::TILE_SIZE - SCREEN_WIDTH;
    int maxCameraY = map.getMAP_HEIGHT() * Map::TILE_SIZE - SCREEN_HEIGHT;
    if (maxCameraX < 0) maxCameraX = 0;
    if (maxCameraY < 0) maxCameraY = 0;

    // クランプ処理
    cameraX = (cameraX_in < 0) ? 0 : (cameraX_in > maxCameraX ? maxCameraX : cameraX_in);
    cameraY = (cameraY_in < 0) ? 0 : (cameraY_in > maxCameraY ? maxCameraY : cameraY_in);
    
    const int SCROLL_SNAP = 8;
    cameraX = (cameraX / SCROLL_SNAP) * SCROLL_SNAP;
    cameraY = (cameraY / SCROLL_SNAP) * SCROLL_SNAP;

    // ★ 分割描画ループ (上から下へ 32px ずつ)
    for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY += BLOCK_HEIGHT) {
        
        // 1. 帯スプライトをクリア
        // (fillScreenは遅いので、全画面描画するなら不要だが、念のため)
        // lineSprite.fillScreen(TFT_BLACK); 
        
        // 2. この帯に含まれるマップ座標を計算
        int worldY_Start = cameraY + drawY;
        int worldY_End   = worldY_Start + BLOCK_HEIGHT; // (含まれない上限)
        
        int tileY_Start = worldY_Start / Map::TILE_SIZE;
        int tileY_End   = (worldY_End - 1) / Map::TILE_SIZE; // 境界調整
        
        int tileX_Start = cameraX / Map::TILE_SIZE;
        int tileX_End   = (cameraX + SCREEN_WIDTH - 1) / Map::TILE_SIZE;

        // 3. タイルを描画 (スプライトに対して)
        for (int ty = tileY_Start; ty <= tileY_End; ++ty) {
          for (int tx = tileX_Start; tx <= tileX_End; ++tx) {
            if (tx < 0 || ty < 0 || tx >= map.getMAP_WIDTH() || ty >= map.getMAP_HEIGHT()) {
              // 範囲外は黒
              // (本来はここで黒埋めが必要だが、map外に出ない前提なら省略可)
              continue;
            }

            int tileType = map.getTile(tx, ty);
            int drawX = tx * Map::TILE_SIZE - cameraX;
            int drawY_local = ty * Map::TILE_SIZE - worldY_Start; // スプライト内での相対Y座標

            // 常に草を描いてから、透過物を重ねる
            const uint16_t* currentGrassBmp = tileBitmapFor(Map::TILE_GRASS);
            lineSprite.pushImage(drawX, drawY_local, Map::TILE_SIZE, Map::TILE_SIZE, currentGrassBmp);
            if (tileType != Map::TILE_GRASS) {
              const uint16_t* bmp = tileBitmapFor(tileType);
              lineSprite.pushImage(drawX, drawY_local, Map::TILE_SIZE, Map::TILE_SIZE, bmp, H_TRANSPARENT);
            }
          }
        }
        // マップが画面幅より小さい場合だけ、
        // 右側のマップ外領域を黒で消す。
        // ラインスプライト全体はクリアしない。
        int mapRight =
            map.getMAP_WIDTH() * Map::TILE_SIZE
            - cameraX;

        if (mapRight < SCREEN_WIDTH) {
          if (mapRight < 0) {
            mapRight = 0;
          }

          lineSprite.fillRect(
              mapRight,
              0,
              SCREEN_WIDTH - mapRight,
              BLOCK_HEIGHT,
              TFT_BLACK
          );
        }

        // 4. この帯にヒーローが居るか？ (重なり判定)
        int heroScreenX = hero.X - cameraX;
        int heroScreenY = hero.Y - cameraY;
        
        // ヒーローの矩形 (32x32)
        // 描画帯のY範囲: [drawY, drawY + BLOCK_HEIGHT)
        // ヒーローのY範囲: [heroScreenY, heroScreenY + 32)
        
        if (heroScreenY < drawY + BLOCK_HEIGHT && heroScreenY + Map::TILE_SIZE > drawY) {
          // 重なっているので描画する
          int heroLocalY = heroScreenY - drawY; // スプライト内Y
          const uint16_t* heroBmp = getHeroImage(hero.direction);
          lineSprite.pushImage(heroScreenX, heroLocalY, Map::TILE_SIZE, Map::TILE_SIZE, (uint16_t*)heroBmp, H_TRANSPARENT);
        }

        // 5. 画面に転送
        lineSprite.pushSprite(0, drawY);
    }

    // 前回の位置・向きを更新
    int hx = hero.X - cameraX;
    int hy = hero.Y - cameraY;

    prevHeroScreenX = hx;
    prevHeroScreenY = hy;
    prevHeroDirection = hero.direction;
    
    // 再描画完了フラグ
    startTileX = 0; // -1 以外なら何でもOK
  }

void Graphic::restoreRectDirect(Map &map, int sx, int sy) {
     // 画面外チェック
     if (sx < 0 || sy < 0 || sx >= SCREEN_WIDTH || sy >= SCREEN_HEIGHT) return;

     int tileL = (sx + cameraX) / Map::TILE_SIZE;
     int tileT = (sy + cameraY) / Map::TILE_SIZE;
     int tileR = (sx + Map::TILE_SIZE - 1 + cameraX) / Map::TILE_SIZE;
     int tileB = (sy + Map::TILE_SIZE - 1 + cameraY) / Map::TILE_SIZE;
     
     // 高速化のため startWrite を使用
     lcd.startWrite();
     for(int ty=tileT; ty<=tileB; ty++) {
        for(int tx=tileL; tx<=tileR; tx++) {
            if (tx < 0 || ty < 0 || tx >= map.getMAP_WIDTH() || ty >= map.getMAP_HEIGHT()) continue;
            
            int screenX = tx * Map::TILE_SIZE - cameraX;
            int screenY = ty * Map::TILE_SIZE - cameraY;
            int tileType = map.getTile(tx, ty);

            // 背景を復元 (直接描画)
            const uint16_t* currentGrassBmp = tileBitmapFor(Map::TILE_GRASS);
            lcd.pushImage(screenX, screenY, Map::TILE_SIZE, Map::TILE_SIZE, currentGrassBmp);
            if (tileType != Map::TILE_GRASS) {
                const uint16_t* bmp = tileBitmapFor(tileType);
                lcd.pushImage(screenX, screenY, Map::TILE_SIZE, Map::TILE_SIZE, bmp, H_TRANSPARENT);
            }
        }
     }
     lcd.endWrite();
  }

void Graphic::smartRender(Map &map, const Caractor &hero, int desiredCameraX, int desiredCameraY){
    // カメラ計算 (drawFullBackgroundと同じロジック)
    int maxCameraX = map.getMAP_WIDTH() * Map::TILE_SIZE - SCREEN_WIDTH;
    int maxCameraY = map.getMAP_HEIGHT() * Map::TILE_SIZE - SCREEN_HEIGHT;
    if (maxCameraX < 0) maxCameraX = 0;
    if (maxCameraY < 0) maxCameraY = 0;
    int camX = (desiredCameraX < 0) ? 0 : (desiredCameraX > maxCameraX ? maxCameraX : desiredCameraX);
    int camY = (desiredCameraY < 0) ? 0 : (desiredCameraY > maxCameraY ? maxCameraY : desiredCameraY);
    const int SCROLL_SNAP = 8;
    camX = (camX / SCROLL_SNAP) * SCROLL_SNAP;
    camY = (camY / SCROLL_SNAP) * SCROLL_SNAP;

    // カメラが動いたら全描画 (ラインバッファ使用)
    // startTileX == -1 は強制再描画フラグ
    if (camX != cameraX || camY != cameraY || startTileX == -1) {
      drawFullBackground(map, hero, camX, camY); // ★ hero を渡す
      return;
    }

    int heroScreenX = hero.X - cameraX;
    int heroScreenY = hero.Y - cameraY;

    // 位置も向きも変わっていなければ何もしない
    if (prevHeroScreenX == heroScreenX &&
        prevHeroScreenY == heroScreenY &&
        prevHeroDirection == hero.direction)
    {
      return;
    }

    // ★ ヒーローの「移動前」と「移動後」の領域だけを復元・描画する
    // 1. 古い場所を背景で塗りつぶす (LCD直接)
    restoreRectDirect(map, prevHeroScreenX, prevHeroScreenY);
    
    // 2. 新しい場所にヒーローを描く (LCD直接)
    // (背景との合成が必要だが、簡易的に透過描画)
    const uint16_t* heroBmp = getHeroImage(hero.direction);
    //lcd.pushImage(heroScreenX, heroScreenY, Map::TILE_SIZE, Map::TILE_SIZE, (uint16_t*)hero_dots, H_TRANSPARENT);
    lcd.pushImage(heroScreenX, heroScreenY, Map::TILE_SIZE, Map::TILE_SIZE, (uint16_t*)heroBmp, H_TRANSPARENT);
    prevHeroScreenX = heroScreenX;
    prevHeroScreenY = heroScreenY;
    prevHeroDirection = hero.direction;
  }

void Graphic::invalidateAndRedraw(Map &map, const Caractor &hero, int desiredCameraX, int desiredCameraY){
    startTileX = -1; // フラグを立てる
    drawFullBackground(map, hero, desiredCameraX, desiredCameraY); // ★ hero を渡す
  }
