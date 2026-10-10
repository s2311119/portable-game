// main.ino
#include <SPI.h>
#include <SdFat.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "bmp.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "pico/sync.h"
critical_section_t sdLock;

#define LGFX_USE_SD 1
#define H_TRANSPARENT 0xFFFF
#include <malloc.h>
// -------------------------
// 変更が必要なピン設定（回路依存）
// -------------------------
static const int SD_CS_PIN = 22;    // SD カード CS
static const int SD_MISO_PIN = 16;  // SD MISO（プルアップ必須）
static const int SD_SCK_PIN  = 18;
static const int SD_MOSI_PIN = 19;

static const int DAC_CS_PIN   = 17; // MCP4921 CS
static const int DAC_SCK_PIN  = 9;  // ソフトSPIクロック
static const int DAC_MOSI_PIN = 8;  // ソフトSPI MOSI

// -------------------------
// Map / Game / Graphic 等（省略せずそのまま）
// -------------------------
// ここは質問者が既に持っているゲームロジック部分をそのまま使います。
// （読みやすさのため必要最小限のみ再掲します）
// main.ino

#include <vector>      // std::vector を使うために必要
#include <algorithm>   // std::min, std::max を使うために必要

//-----------------------------
/*
ストライカー系: 39->44->49->{54, 55} ->{{61, 62}, {63, 64}}
ガーディアン系: 40->45->50->{56, 57} ->{{65, 66}, {67. 68}}
クリッパー系: 41->46->51->58 ->{69, 70}
メンダ―系: 42->47->52->59 ->{71, 72}
ジャマー系: 43->48->53->60->{73, 74}
*/
//-----------------------------

// グローバル（core0 から参照するけど SD 操作は core1 のみ）
enum EventType {
  EVENT_NONE,
  EVENT_TREASURE_BOX,
  EVENT_STAIR_UP,
  EVENT_STAIR_DOWN,
  EVENT_AUTOMATON,
  EVENT_INN,     // ★追加
  EVENT_SHOP,    // ★追加
  EVENT_STORAGE,  // ★追加
  EVENT_BOSS_BATTLE
};

enum GameState {
  STATE_GAME,
  STATE_MENU, 
  STATE_BATTLE,
  STATE_EVENT,
  STATE_GAME_OVER
};
#include <malloc.h> // これが必要です

class MemoryMonitor {
private:
  unsigned long lastCheckTime = 0;
  const unsigned long INTERVAL = 10000; // 10秒 (10000ms)

public:
  void update() {
    unsigned long now = millis();
    if (now - lastCheckTime >= INTERVAL) {
      lastCheckTime = now;
      printInfo();
    }
  }

  void printInfo() {
    struct mallinfo m = mallinfo();
    
    // RP2040のRAMは約264KBですが、プログラム領域などを除いた
    // ヒープとして使える領域の統計を表示します。
    
    Serial.println("=== Memory Status (10s) ===");
    Serial.printf("Used (使用中): %d bytes (%.2f KB)\n", m.uordblks, m.uordblks / 1024.0);
    Serial.printf("Free (空き)  : %d bytes (%.2f KB)\n", m.fordblks, m.fordblks / 1024.0);
    
    // メモリ断片化の目安（空きがあるのに大きい確保ができない原因）
    // これが小さいと、大きな連続したメモリ(画像など)が確保できません
    // Serial.printf("Max Free Block: %d bytes\n", m.mxordblk); // (環境によっては0になる場合あり)
    
    Serial.println("===========================");
  }
};

class Map {
public:
  int MAP_WIDTH;
  int MAP_HEIGHT;
  int automatonWhoId;
  int currentFloor = 1;
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
  // マップデータを保持する動的配列
  std::vector<uint8_t> mapData;

  enum MapType {
    TYPE_MAZE,
    TYPE_DUNGEON,
    TYPE_TOWN,
    TYPE_BOSS_ROOM
  };
  MapType currentType;
  bool isTown = false;
  // ★★★ 追加：部屋の矩形情報を保存する構造体 ★★★
  struct Room {
    int x, y, w, h;
  };

  struct ObjectPos {
    int x, y;
  };

  std::vector<ObjectPos> boxPositions; // 宝箱の位置リスト
  ObjectPos stairUpPos = {-1, -1};    // 上り階段の位置
  ObjectPos stairDownPos = {-1, -1};  // 下り階段の位置
  ObjectPos playerStartPos; //主人公の初期位置

  // ★★★ 修正後のコンストラクタ ★★★
  Map(int max_w, int max_h, MapType type)
  {
    // 初期化リスト (:) ではなく、ここで代入する
    currentType = type;
    /*MAP_WIDTH = ((type != TYPE_MAZE && type != TYPE_DUNGEON) ? random(4, max_w / 2) * 2 + 1 : random(15, max_w / 2) * 2 + 1);
    MAP_HEIGHT = ((type != TYPE_MAZE && type != TYPE_DUNGEON) ? random(4, max_h / 2) * 2 + 1 : random(15, max_h / 2) * 2 + 1);*/
    if (type == TYPE_TOWN) {
      // 集落: 床5x5 + 周囲の壁2 = 7x7
      MAP_WIDTH = 8;
      MAP_HEIGHT = 8;
    } 
    else if (type == TYPE_BOSS_ROOM) {
      // ボス部屋: 床7x7 + 周囲の壁2 = 9x9
      MAP_WIDTH = 9;
      MAP_HEIGHT = 9;
    }
    else {
      // ダンジョンは最低15マスとし、
      // 指定された最大サイズ以下の奇数サイズにする
      const int MIN_DUNGEON_SIZE = 15;

      int safeMaxW =
          (max_w < MIN_DUNGEON_SIZE)
              ? MIN_DUNGEON_SIZE
              : max_w;

      int safeMaxH =
          (max_h < MIN_DUNGEON_SIZE)
              ? MIN_DUNGEON_SIZE
              : max_h;

      // 最大値も奇数にそろえる
      int maxOddW =
          (safeMaxW % 2 == 0)
              ? safeMaxW - 1
              : safeMaxW;

      int maxOddH =
          (safeMaxH % 2 == 0)
              ? safeMaxH - 1
              : safeMaxH;

      // 15, 17, 19, ... の中から抽選
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
    mapData.assign(MAP_WIDTH * MAP_HEIGHT, TILE_WALL);
    generateMazeAndRooms(type);
    placeObjects();
  }
  int getMAP_WIDTH(){
    return MAP_WIDTH;
  }
  int getMAP_HEIGHT(){
    return MAP_HEIGHT;
  }
  void regenerate(int max_w, int max_h, MapType type){
    // public側のMAP_WIDTH/HEIGHTを変更する
    currentType = type;
    
    isTown = false;
    if (type == TYPE_TOWN) {
      // 集落: 床5x5 + 周囲の壁2 = 7x7
      MAP_WIDTH = 8;
      MAP_HEIGHT = 8;
      isTown = true;
    } 
    else if (type == TYPE_BOSS_ROOM) {
      // ボス部屋: 床7x7 + 周囲の壁2 = 9x9
      MAP_WIDTH = 9;
      MAP_HEIGHT = 9;
    }

    else {
      // ダンジョンは最低15マスとし、
      // 指定された最大サイズ以下の奇数サイズにする
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

    mapData.assign(MAP_WIDTH * MAP_HEIGHT, TILE_WALL);
    if (type == TYPE_BOSS_ROOM) {
      // --- ボス部屋の描画 ---
      
      // 全体を床にする
      for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) mapData[i] = TILE_GRASS;
      
      // 周囲を壁にする
      for (int x = 0; x < MAP_WIDTH; x++) { 
          mapData[x] = TILE_WALL; 
          mapData[(MAP_HEIGHT-1)*MAP_WIDTH + x] = TILE_WALL; 
      }
      for (int y = 0; y < MAP_HEIGHT; y++) { 
          mapData[y*MAP_WIDTH] = TILE_WALL; 
          mapData[y*MAP_WIDTH + (MAP_WIDTH-1)] = TILE_WALL; 
      }

      // 下り階段 (入り口)
      // 座標が範囲外にならないよう中央下付近に固定
      int stairX = MAP_WIDTH / 2;
      int stairY = MAP_HEIGHT - 3;
      mapData[stairY * MAP_WIDTH + stairX] = TILE_STAIR_DOWN;
      
      // プレイヤー開始位置
      playerStartPos = {stairX, stairY};

      // ★ボス配置 (部屋の中央奥)
      int bossX = MAP_WIDTH / 2;
      int bossY = 5;
      mapData[bossY * MAP_WIDTH + bossX] = TILE_BOSS;

      return; // ボス部屋生成完了
    }
    // メンバー変数を再利用して再生成
    generateMazeAndRooms(type);
    placeObjects();
  }
private:
  //int MAP_WIDTH;
  //int MAP_HEIGHT;
  std::vector<int> pathX, pathY;
  std::vector<int> dirs;
  std::vector<Room> placedRooms;
  std::vector<ObjectPos> grassTiles;
  void generateMazeAndRooms(MapType type) {
    if (type == TYPE_TOWN) {
      // 1. 全体を壁で初期化 (コンストラクタ等で既にされているが念のため)
      // mapData.assign(MAP_WIDTH * MAP_HEIGHT, TILE_WALL); 

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
    if (currentFloor >= 21 && random(100) < 5) { // 例: 5%の確率
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

  // (generateDungeon のヘルパー関数) 部屋を掘る
  void carveRoom(int x1, int y1, int x2, int y2) {
    for (int y = y1; y < y2; y++) {
      for (int x = x1; x < x2; x++) {
        if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
          mapData[y * MAP_WIDTH + x] = TILE_GRASS;
        }
      }
    }
  }

  void placeObjects() {
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
      if (currentFloor > 1 && !grassTiles.empty()) {
        idx = random((int)grassTiles.size());
        stairDownPos = grassTiles[idx];
        mapData[stairDownPos.y * MAP_WIDTH + stairDownPos.x] = TILE_STAIR_DOWN;
        grassTiles[idx] = grassTiles.back(); grassTiles.pop_back();
      }
      if (currentFloor > 1){
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

  // ★★★ 追加: 迷路生成後に宝箱と階段を配置する関数 ★★★
  
};

class Item {
public:
  enum ItemType {
    ITEM_CONSUMABLE, // 消費アイテム (回復薬など)
    ITEM_WEAPON,     // 武器
    ITEM_ARMOR,      // 防具
    ITEM_ACCESSORY,  // アクセサリー
    ITEM_MATERIAL,   // 素材 (換金用)
    ITEM_IMPORTANT   // 大事なもの
  };
  ItemType type; // ★ 追加: 種類
  int value;     // ★ 追加: 威力 (武器ならATK、防具ならDEF、薬なら回復量)
  int price;     // ★ 追加: 買値 (売値はこの半額にするのが一般的)
  int id;
  const char* name;

  // コンストラクタ
  Item(int _id = 0, const char* _name = "", ItemType _type = ITEM_MATERIAL, int _value = 0, int _price = 0) {
    id = _id;
    name = _name;
    type = _type;
    value = _value;
    price = _price;
  }
};

class Skill {
public:
  int id;
  const char* name;
  int tpCost; // ★消費TP (ゼンマイ)
  int power;  // ★技の威力 (Senseステータスで計算)
  enum class StatDependence {
    NONE, // 依存なし (固定値など)
    ATK,  // 物理 (Attack vs Defense)
    SEN   // 魔法/技術 (Sense vs Sense)
  };
  // main.ino (Skill クラスのコンストラクタ 322行目あたり)

  enum class EffectType {
    NONE,             // 効果なし
    DAMAGE,           // 物理・属性ダメージ (powerを参照)
    HEAL,             // HP回復 (powerを参照)
    CURE_STATUS,      // 状態異常の治療 (例: キュア)
    BUFF,             // 強化 (例: パワーチャージ, クイックステップ)
    DEBUFF,           // 弱体化 (例: スロウ, パワーダウン)
    REVIVE,           // 蘇生 (例: リザレクション)
    MULTI_HIT_DAMAGE, // 多段ヒット (例: ツインエッジ)
    SPECIAL           // その他 (例: 逃走, 仲間を呼ぶ, 複合効果など)
  };

  // スキルの「対象範囲」
  enum class TargetScope {
    SELF,             // 使用者自身
    SINGLE_ALLY,      // 味方単体
    SINGLE_ENEMY,     // 敵単体
    ALL_ALLIES,       // 味方全体
    ALL_ENEMIES,      // 敵全体
    RANDOM_ENEMY      // 敵ランダム (複数回ヒット用など)
  };
  EffectType type;      // スキルの効果タイプ
  TargetScope scope;    // スキルの対象範囲
  StatDependence dependence;
  enum class Category {
    PHYSICAL, // 物理
    MELODY,   // 旋律 (魔法)
    OTHER     // その他 (バフなど)
  };
  Category category;

  Skill(int recid) {
    id = recid;

    // デフォルト値
    name = "？？？";
    tpCost = 0;
    power = 0;

    type = EffectType::DAMAGE;
    scope = TargetScope::SINGLE_ENEMY;
    dependence = StatDependence::ATK;
    category = Category::PHYSICAL;

    switch (id) {
      // --- 主人公 (ID 0-19) ---
      case 0: name = "プチノイズ"; tpCost = 5; power = 10; dependence = StatDependence::SEN; break; // 魔法っぽいのでSEN
      case 1: name = "ノイズ・ブラスト"; tpCost = 3; power = 60; dependence = StatDependence::SEN; break;
      case 2: name = "レゾナンス・ウェイブ"; tpCost = 6; power = 40; scope = TargetScope::ALL_ENEMIES; dependence = StatDependence::SEN; break;
      case 3: name = "ディソナンス"; tpCost = 14; power = 120; dependence = StatDependence::SEN; break;
      // (回復・補助系は dependence はあまり関係ないが、回復量はSEN依存にするのが一般的)
      case 4: name = "ファースト・エイド"; tpCost = 8; power = 40; type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; dependence = StatDependence::SEN; break;
      case 5: name = "ピュリファイ"; tpCost = 10; power = 0; type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; break;
      case 6: name = "ヒーリング・サークル"; tpCost = 20; power = 80; type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; dependence = StatDependence::SEN; break;
      case 7: 
        name = "チューニング・アタック"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // 味方単体ATKアップ
      case 8: 
        name = "チューニング・ガード"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // 味方単体DEFアップ
      case 9: 
        name = "アレグロ"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // 味方全体SPDアップ
      case 10: 
        name = "ラレンタンド"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // 敵単体SPDダウン
      case 11: 
        name = "ノイズ・ジャミング"; tpCost = 10; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // 敵全体ATKダウン
      // (バフ・デバフ系は威力0なので省略、奥義はSEN)
      case 12: name = "グランド・シンフォニー"; tpCost = 60; power = 300; scope = TargetScope::ALL_ENEMIES; dependence = StatDependence::SEN; break;
      case 13: name = "完全調律"; tpCost = 60; power = 999; type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; dependence = StatDependence::SEN; break;
      // --- ストライカー (who=39) ---
      case 20: 
        name = "ハードヒット"; tpCost = 5; power = 80; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 21: 
        name = "パワーチャージ"; tpCost = 8; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK 1.5倍バフなど)

      // --- ギア・ソルジャー (who=40) ---
      case 22: 
        name = "スラッシュ"; tpCost = 8; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 23: 
        name = "ダブルヒット"; tpCost = 10; power = 100; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (30ダメージ x 2回)

      // --- ギア・ナイト (who=41) ---
      case 24: 
        name = "アーマーブレイク"; tpCost = 10; power = 150; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (防御無視 or 防御ダウン)
      case 25: 
        name = "ソードダンス"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK/SPD バフ)

      // --- ヴァンガード (who=42) ---
      case 26: 
        name = "シールドスマッシュ"; tpCost = 20; power = 300; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 27: 
        name = "かばう"; tpCost = 5; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (仲間を守る)

      // --- スワッシュバックラー (who=45) ---
      case 28: 
        name = "ファントムエッジ"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (クリティカル率 高)
      case 29: 
        name = "トリックステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (SPD/LCK バフ)
      
      // --- オーバーロード (who=43) ---
      case 30: 
        name = "ギガブレイク"; tpCost = 40; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 31: 
        name = "王者の波動"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 ATK/DEF バフ)
      case 32: 
        name = "キングス・シールド"; tpCost = 10; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (物理・旋律反射)
      case 33: 
        name = "デッドエンド"; tpCost = 60; power = 600; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;

      // --- デストロイヤー (who=44) ---
      case 34: 
        name = "バーサーク"; tpCost = 3; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK 2倍 / DEF 0.5倍)
      case 35: 
        name = "カラミティ・エンド"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (HPが低いほど高威力)
      case 36: 
        name = "トリプルスラッシュ"; tpCost = 15; power = 150; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (80ダメージ x 3回)
      case 37: 
        name = "デモリッシュ"; tpCost = 55; power = 600;
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;

      // --- ゼロ・ブレード (who=46) ---
      case 38: 
        name = "ソニックブレイド"; tpCost = 10; power = 100; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制)
      case 39: 
        name = "ミラーズエッジ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (回避率 2倍バフ)
      case 40: 
        name = "刹那"; tpCost = 20; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高クリティカル率)
      case 41: 
        name = "無空"; tpCost = 65; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (TP消費が激しい)

      // --- ヴォイド・ストーカー (who=47) ---
      case 42: 
        name = "アサシネイト"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (確率で即死)
      case 43: 
        name = "ダーククローク"; tpCost = 3; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (姿を消す / 狙われない)
      case 44: 
        name = "ペインサイクル"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (与ダメージ分 HP 回復)
      case 45: 
        name = "ヴォイド・ショック"; tpCost = 45; power = 500; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高確率で状態異常)

      // --- ガーディアン (who=1) ---
      case 46: 
        name = "プロテクト"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 DEF 1.5倍)
      case 47: 
        name = "シールドバッシュ"; tpCost = 10; power = 80; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (威力20 + 確率でスタン)

      // --- フォートレス (who=6) ---
      case 48: 
        name = "ウォール"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 DEF 1.2倍)
      case 49: 
        name = "挑発"; tpCost = 10; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (敵の攻撃を自分に集める)

      // --- キャッスル・ウォール (who=11) ---
      case 50: 
        name = "リフレクト"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (物理攻撃を一度だけ反射)
      case 51: 
        name = "ガーディアン魂"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (DEF 2倍、SPD 0.5倍)

      // --- バスティオン (who=17) ---
      case 52: 
        name = "鉄壁"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (3ターン DEF 2.5倍)
      case 53: 
        name = "カウンター"; tpCost = 5; power = 100; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (受けたダメージを反撃)

      // --- イージス (who=18) ---
      case 54: 
        name = "リペア"; tpCost = 5; power = 60; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 回復)
      case 55: 
        name = "フォースフィールド"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 ダメージ無効 1回)

      // --- アダマンタイト (who=26) ---
      case 56: 
        name = "アダマン・ボディ"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (物理ダメージを 1 に)
      case 57: 
        name = "自己修復"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (HPリジェネ(大))
      case 58: 
        name = "ラストスタンド"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (HP1で耐える)
      case 59: 
        name = "ギガンティック・ウォール"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 DEF 2倍)

      // --- インヴィンシブル (who=27) ---
      case 60: 
        name = "超回復"; tpCost = 0; power = 500; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (HP大回復)
      case 61: 
        name = "オートリバイブ"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自動蘇生 1回)
      case 62: 
        name = "最大HP上昇"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (戦闘中 MaxHP 1.5倍)
      case 63: 
        name = "無敵"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (1ターン ダメージ 0)

      // --- ディヴァイン・シールド (who=28) ---
      case 64: 
        name = "全体バリア"; tpCost = 15; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 ダメージ無効 1回)
      case 65: 
        name = "旋律反射"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (旋律(魔法)反射)
      case 66: 
        name = "キュア・オール"; tpCost = 20; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 状態異常回復)
      case 67: 
        name = "ディバイン・ウォール"; tpCost = 20; power = 0;
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (効果: 全ダメージ軽減)
      
      // --- リジェネレーター (who=29) ---
      case 68: 
        name = "全体リペア"; tpCost = 10; power = 1000; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 回復)
      case 69: 
        name = "リザレクション"; tpCost = 0; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 蘇生)
      case 70: 
        name = "TPリジェネ"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のTPを徐々に回復)
      case 71: 
        name = "フル・キュア"; tpCost = 10; power = 9999; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 HP全回復)

      // --- クリッパー (who=41) ---
      case 72: 
        name = "カッティング"; tpCost = 7; power = 80; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 73: 
        name = "クイックステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 SPD バフ)

      // --- ブレード・ダンサー (who=46) ---
      case 74: 
        name = "ツインエッジ"; tpCost = 10; power = 60; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (35ダメージ x 2回)
      case 75: 
        name = "エアリアルダンス"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 AGI バフ)

      // --- サイクロン・ダンサー (who=51) ---
      case 76: 
        name = "スパイラルエッジ"; tpCost = 15; power = 100; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (70ダメージ x 2回)
      case 77: 
        name = "エアステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 SPD/AGI バフ)

      // --- テンペスト (who=58) ---
      case 78: 
        name = "ゲイルストライク"; tpCost = 5; power = 140; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (風属性)
      case 79: 
        name = "ラピッドムーブ"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 SPD バフ)

      // --- シルフィード (who=69) ---
      case 80: 
        name = "インビジブルエッジ"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (回避無視)
      case 81: 
        name = "ウインドウォール"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 回避率UP)
      case 82: 
        name = "ストームブリンガー"; tpCost = 10; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::ALL_ENEMIES; 
        break; // (全体 風属性攻撃)
      case 83: 
        name = "精霊の舞"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のTP徐々に回復)

      // --- アクセラレーター (who=70) ---
      case 84: 
        name = "マッハブレイド"; tpCost = 13; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制)
      case 85: 
        name = "オーバークロック"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 2回行動)
      case 86: 
        name = "リミットブレイク"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のATK/SPD 最大まで上昇)
      case 87: 
        name = "タービュランス"; tpCost = 30; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::ALL_ENEMIES; 
        break; // (全体 風属性 + SPDダウン)

      // --- メンダー (who=42) ---
      case 88: 
        name = "リペア"; tpCost = 5; power = 50; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復)
      case 89: 
        name = "キュア"; tpCost = 5; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 状態異常回復)

      // --- (第1進化: who=47) ---
      case 90: 
        name = "リペア・プラス"; tpCost = 10; power = 150; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復(中))
      case 91: 
        name = "リフレッシュ"; tpCost = 5; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 弱体解除)

      // --- (第2進化: who=52) ---
      case 92: 
        name = "エリア・リペア"; tpCost = 15; power = 100; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP回復(小))
      case 93: 
        name = "リザレクション"; tpCost = 10; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 蘇生(HP中))

      // --- (第3進化: who=59) ---
      case 94: 
        name = "フル・リペア"; tpCost = 15; power = 500; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復(大))
      case 95: 
        name = "オートリペア"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP自動回復)

      // --- (最終進化A: who=71) ---
      case 96: 
        name = "エリア・リペア・プラス"; tpCost = 25; power = 250; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP回復(中))
      case 97: 
        name = "リジェネフィールド"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP自動回復)
      case 98: 
        name = "アセンション"; tpCost = 60; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 蘇生(HP全快))
      case 99: 
        name = "ホーリー・ウォール"; tpCost = 20; power = 0;
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 状態異常無効 1回)

      // --- (最終進化B: who=72) ---
      case 100: 
        name = "TPチャージ"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 TP回復)
      case 101: 
        name = "ディスペル"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 強化解除)
      case 102: 
        name = "TPリジェネ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 TP自動回復)
      case 103: 
        name = "オラクルフィールド"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 TP徐々に回復)

      // --- ジャマー (who=43) ---
      case 104: 
        name = "スロウ"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 SPD ダウン)
      case 105: 
        name = "ポイズン"; tpCost = 5; power = 50; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (ダメージ + 確率で毒)

      // --- (第1進化: who=48) ---
      case 106: 
        name = "パワーダウン"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 ATK ダウン)
      case 107: 
        name = "ガードダウン"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 DEF ダウン)

      // --- (第2進化: who=53) ---
      case 108: 
        name = "エリア・スロウ"; tpCost = 10; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 SPD ダウン)
      case 109: 
        name = "サイレンス"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 確率で沈黙)

      // --- (第3進化: who=60) ---
      case 110: 
        name = "デッドリーポイズン"; tpCost = 10; power = 400; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (ダメージ + 確率で猛毒)
      case 111: 
        name = "TPドレイン"; tpCost = 5; power = 380; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 TP吸収)

      // --- (最終進化A: who=73) ---
      case 112: 
        name = "エリア・ブレイク"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 ATK/DEF ダウン)
      case 113: 
        name = "カオスフィールド"; tpCost = 20; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 確率で混乱)
      case 114: 
        name = "ディザスター"; tpCost = 30; power = 1500; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (無属性 + 全状態異常付与)
      case 115: 
        name = "ペイン・イーター"; tpCost = 30; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (状態異常の敵に特効)

      // --- (最終進化B: who=74) ---
      case 116: 
        name = "ロックダウン"; tpCost = 0; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 確率で行動不能)
      case 117: 
        name = "バニッシュ"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 強化全解除)
      case 118: 
        name = "ワールド・ダウン"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 全能力ダウン)
      case 119: 
        name = "エナジー・バーン"; tpCost = 50; power = 300; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵のTPが多いほど高威力)

      // --- はぐれノイズ (仲間用) (who=75) ---
      case 120: 
        name = "ノイズヒット"; tpCost = 0; power = 90; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (シンプルな攻撃)
      case 121: 
        name = "ソニックダガー"; tpCost = 0; power = 120; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制攻撃)
      case 122:
        name = "かく乱"; tpCost = 0; power = 90;
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY;
        break; // (攻撃 + 確率で混乱)
      case 123: 
        name = "ファントムラッシュ"; tpCost = 0; power = 130; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::RANDOM_ENEMY; 
        break; // (40ダメージ x 2-4回ランダム攻撃)

      // --- はぐれキング (仲間用) (who=76) ---
      case 124: 
        name = "キングスピア"; tpCost = 0; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (防御無視攻撃)
      case 125: 
        name = "ノイズストーム"; tpCost = 0; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体攻撃)
      case 126: 
        name = "ロイヤルチャージ"; tpCost = 0; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高威力だが次のターン行動不可)
      case 127: 
        name = "ラッキーセブン"; tpCost = 0; power = 777; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::RANDOM_ENEMY; 
        break; // (77ダメージ x 1-7回ランダム攻撃)

      default:
        name = "？？？";
        tpCost = 0;
        power = 0;
        type = EffectType::NONE; scope = TargetScope::SINGLE_ENEMY; 
        break;
    }
    // メンダー系 (88-103) は全て Sense 依存
    if (id >= 88 && id <= 103) {
        dependence = StatDependence::SEN;
    }
    // ジャマー系 (104-119) も全て Sense 依存
    if (id >= 104 && id <= 119) {
        dependence = StatDependence::SEN;
    }
    // 主人公の攻撃スキル (1-3, 12)
    if (id <= 3 || id == 12) {
        dependence = StatDependence::SEN;
    }
    // はぐれ系 (125 ノイズストーム, 126 ロイヤルチャージ) は魔法っぽい？
    if (id == 125 || id == 126) {
        dependence = StatDependence::SEN;
    }
    // スキル設定がすべて確定した後にカテゴリを決定
    bool isOffensive =
        type == EffectType::DAMAGE ||
        type == EffectType::MULTI_HIT_DAMAGE ||
        (type == EffectType::SPECIAL &&
        power > 0 &&
        (scope == TargetScope::SINGLE_ENEMY ||
          scope == TargetScope::ALL_ENEMIES ||
          scope == TargetScope::RANDOM_ENEMY));

    if (!isOffensive) {
        category = Category::OTHER;
    }
    else if (dependence == StatDependence::SEN) {
        category = Category::MELODY;
    }
    else {
        category = Category::PHYSICAL;
    }
  }
};

// main.ino

Item createItemById(int id) {
  
  // --- 0. 素材・換金アイテム (0-99) ---
  
  // 基本素材
  if (id == 1) return Item(1, "鉄くず", Item::ITEM_MATERIAL, 0, 10);
  if (id == 2) return Item(2, "銅の歯車", Item::ITEM_MATERIAL, 0, 30); // 名前修正(銅の歯車)
  
  // 進化アイテム (値段は適当に設定、売れるようにする)
  if (id == 10) return Item(10, "攻撃の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 11) return Item(11, "赤の歯車",   Item::ITEM_MATERIAL, 0, 100);
  if (id == 12) return Item(12, "紅の歯車",   Item::ITEM_MATERIAL, 0, 500);
  if (id == 13) return Item(13, "防御の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 14) return Item(14, "青の歯車",   Item::ITEM_MATERIAL, 0, 100);
  if (id == 15) return Item(15, "蒼の歯車",   Item::ITEM_MATERIAL, 0, 500);
  if (id == 16) return Item(16, "速度の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 17) return Item(17, "回復の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 18) return Item(18, "感応の歯車", Item::ITEM_MATERIAL, 0, 100);

  // (追加: 金の歯車など)
  if (id == 1010) return Item(1010, "銀の歯車", Item::ITEM_MATERIAL, 0, 200); // ID重複避けるなら変える
  // ※ 元コードにあった id=10,11 は上の進化アイテムと被っているので、
  // 銀の歯車などを別のIDにするか、あるいは上記のID構成に合わせる必要があります。
  // 今回は「進化アイテム」を優先し、銀・金は以下のようにします
  if (id == 3) return Item(3, "銀の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 4) return Item(4, "金の歯車", Item::ITEM_MATERIAL, 0, 500);
  if (id == 5) return Item(5, "謎のパーツ", Item::ITEM_MATERIAL, 0, 1000);


  // --- 1. 消費アイテム (100-199) ---
  if (id == 100) return Item(100, "リペアキット", Item::ITEM_CONSUMABLE, 50, 50);      // 50G
  if (id == 101) return Item(101, "リペアキット中", Item::ITEM_CONSUMABLE, 200, 150);  // 150G
  if (id == 102) return Item(102, "リペアキット大", Item::ITEM_CONSUMABLE, 9999, 500); // 500G
  if (id == 103) return Item(103, "エネルギー缶", Item::ITEM_CONSUMABLE, 20, 200);    // TP回復
  if (id == 104) return Item(104, "万能オイル", Item::ITEM_CONSUMABLE, 0, 100);       // 状態異常回復

  // --- 2. 巻物 (200-299) ---
  if (id >= 200 && id < 300) {
    int skillId = id - 200;
    static Skill tempSkill(skillId); 
    tempSkill = Skill(skillId);
    return Item(id, tempSkill.name, Item::ITEM_CONSUMABLE, 0, 500); 
  }

  // --- 3. 武器 (300-399) ---
  if (id == 300) return Item(300, "練習のタクト", Item::ITEM_WEAPON, 5, 50);
  if (id == 301) return Item(301, "鉄のタクト", Item::ITEM_WEAPON, 15, 300);
  if (id == 302) return Item(302, "強化タクト", Item::ITEM_WEAPON, 30, 1000);
  if (id == 303) return Item(303, "白銀のタクト", Item::ITEM_WEAPON, 50, 3500);
  if (id == 304) return Item(304, "近衛兵のバトン", Item::ITEM_WEAPON, 80, 9000);
  
  // ★ 宝箱限定 (最強)
  if (id == 399) return Item(399, "創世のタクト", Item::ITEM_WEAPON, 1500, 20000);

  // --- 4. 防具 (400-499) ---
  if (id == 400) return Item(400, "旅人のコート", Item::ITEM_ARMOR, 3, 30);
  if (id == 401) return Item(401, "レザーコート", Item::ITEM_ARMOR, 10, 250);
  if (id == 402) return Item(402, "強化スーツ", Item::ITEM_ARMOR, 20, 800);
  if (id == 403) return Item(403, "バトルスーツ", Item::ITEM_ARMOR, 35, 3000);
  if (id == 404) return Item(404, "指揮官の軍服", Item::ITEM_ARMOR, 60, 8000);

  // ★ 宝箱限定 (最強)
  if (id == 499) return Item(499, "イージス・ギア", Item::ITEM_ARMOR, 1200, 20000);

  // --- 5. アクセサリー (500-599) ---
  if (id == 500) return Item(500, "パワーリング", Item::ITEM_ACCESSORY, 10, 400); // ATK
  if (id == 501) return Item(501, "ガードリング", Item::ITEM_ACCESSORY, 10, 400); // DEF
  if (id == 502) return Item(502, "クイックブーツ", Item::ITEM_ACCESSORY, 10, 400); // SPD
  if (id == 503) return Item(503, "スナイパーレンズ", Item::ITEM_ACCESSORY, 10, 400); // SEN
  if (id == 504) return Item(504, "ラッキーコイン", Item::ITEM_ACCESSORY, 50, 400); // LCK
  
  // ★ 宝箱限定 (最強)
  if (id == 599) return Item(599, "トリニティ・コア", Item::ITEM_ACCESSORY, 500, 15000);

  // デフォルト
  return Item(id, "不明な品", Item::ITEM_MATERIAL, 0, 1);
}
// main.ino の Item クラスの後に追加

class Inventory {
public:
  // 所持品リスト
  struct ItemStack {
    Item item;
    int quantity;
  };
  std::vector<ItemStack> items;
  int money = 0;
  Inventory() {
    // 起動時に std::vector のメモリを
    // ある程度確保しておくと、後々の追加処理が高速になります
    items.reserve(20); // とりあえず20個分
  }

  // ★アイテムを追加する関数
  void addItem(const Item& newItem) {
    // 既に持っているか探す
    for (auto& stack : items) {
      if (stack.item.id == newItem.id) {
        stack.quantity++; // あったら個数を増やす
        return;
      }
    }
    // なかったら新しく追加 (個数1)
    items.push_back({newItem, 1});
  }

  int countItem(int id) {
    for (const auto& stack : items) {
      if (stack.item.id == id) {
        return stack.quantity;
      }
    }
    return 0;
  }

  bool removeItem(int id) {
    for (auto it = items.begin(); it != items.end(); ++it) {
      if (it->item.id == id) {
        it->quantity--;
        if (it->quantity <= 0) {
          items.erase(it); // 0個になったらリストから消す
        }
        return true;
      }
    }
    return false; // 見つからなかった
  }
  void removeByIndex(int index) {
      if (index < 0 || index >= items.size()) return;
      
      items[index].quantity--;
      if (items[index].quantity <= 0) {
          items.erase(items.begin() + index);
      }
  }
  // (将来的に、アイテム一覧を表示する関数なども追加)
  void saveTo(FsFile& file) {
    file.println(money);
    file.println(items.size());
    for (const auto& stack : items) {
      file.println(stack.item.id);
      file.println(stack.quantity);
    }
  }

  void loadFrom(FsFile& file) {
    money = file.readStringUntil('\n').toInt();
    int count = file.readStringUntil('\n').toInt();
    
    items.clear();
    for (int i = 0; i < count; i++) {
      int id = file.readStringUntil('\n').toInt();
      int qty = file.readStringUntil('\n').toInt();
      // アイテム生成してリストに追加
      Item item = createItemById(id);
      items.push_back({item, qty});
    }
  }
};

const int learnableSkillIDs[] = {
  // --- 攻撃系 ---
  1,  // ノイズ・ブラスト (TP:12)
  2,  // レゾナンス・ウェイブ (TP:25, 全体)
  3,  // ディソナンス (TP:20)
  
  // --- 回復・治療系 ---
  4,  // ファースト・エイド (TP:8)
  5,  // ピュリファイ (TP:10, 治療)
  6,  // ヒーリング・サークル (TP:30, 全体回復)

  // --- 補助・妨害系 ---
  7,  // チューニング・アタック (TP:15, ATK↑)
  8,  // チューニング・ガード (TP:15, DEF↑)
  9,  // アレグロ (TP:15, 全体SPD↑)
  10, // ラレンタンド (TP:12, 敵SPD↓)
  11, // ノイズ・ジャミング (TP:18, 全体ATK↓)

  // --- 奥義 (コスト高め) ---
  12, // グランド・シンフォニー (TP:60, 全体特大)
  13  // 完全調律 (TP:80, 全体全快)
};
const int learnableSkillCount = sizeof(learnableSkillIDs) / sizeof(int);

class Status{
private:
  int HP;
  int originalMaxHP;
  int MaxHP;
  //tune point
  int TP;
  int MaxTP;
  //物理攻撃に関係
  int Attack;
  //防御に関係
  int Defense;
  //回避率、命中率に関係
  int Speed;
  //旋律、ノイズ攻撃に使用
  int Sense;
  // ドロ率、クリ率に関係
  int Luck;
  int Level;
  int who;
  const char* name;
  int currentXP;
  int nextLevelXP;
  std::vector<Skill> learnedSkills; // 習得済みの旋律リスト
  int getPreEvolution(int currentId) {
    for (int i = 0; i < evolutionRuleCount; i++) {
      // 進化テーブルの「進化後(after)」が自分と一致したら、「進化前(before)」を返す
      if (evolutionTable[i].after_who_id == currentId) {
        return evolutionTable[i].before_who_id;
      }
    }
    return -1; // 進化前なし
  }
public:
  int equipWeaponId = -1; // 武器
  int equipArmorId = -1;  // 防具
  int equipAccId = -1;    // アクセ
  // --- 状態異常 ---
  int poisonTurns = 0;
  int poisonLevel = 0;   // 0: なし, 1: 毒, 2: 猛毒
  int paralyzedTurns = 0;
  int silencedTurns = 0;
  int confusedTurns = 0;
  // (他にも必要に応じて追加)
  int coverTurns = 0;
  // --- バフ/デバフ管理 ---
  // (効果量を float で、持続ターンを int で持つ)
    
  // バフ (Rateは倍率, 1.5 = 1.5倍)
  float atkBuffRate = 0;
  int atkBuffTurns = 0;
  float defBuffRate = 0;
  int defBuffTurns = 0;
  float spdBuffRate = 0;
  int spdBuffTurns = 0;
  float senBuffRate = 0;
  int senBuffTurns = 0;
  float lckBuffRate = 0;
  int lckBuffTurns = 0;
  float evaBuffRate = 0; // 回避率
  int evaBuffTurns = 0;

  // デバフ (Rateは倍率, 0.7 = 0.7倍)
  float atkDebuffRate = 0;
  int atkDebuffTurns = 0;
  float defDebuffRate = 0;
  int defDebuffTurns = 0;
  float spdDebuffRate = 0;
  int spdDebuffTurns = 0;
  float senDebuffRate = 0;
  int senDebuffTurns = 0;
  float lckDebuffRate = 0;
  int lckDebuffTurns = 0;

  // (HP/TPリジェネなどの特殊効果)
  int hpRegenTurns = 0;
  int tpRegenTurns = 0;
  // (その他、無敵、反射など)
  int invalidDamageTurns = 0; 
  int reflectPhysicalTurns = 0;
  int reflectMagicTurns = 0;
  int tauntTurns = 0;
  int twoActionsTurns = 0;
  int untargetableTurns = 0;
  int maxHpBuffTurns = 0;
  int physDamageOneTurns = 0;
  int endureTurns = 0;
  int counterTurns = 0;
  int autoReviveTurns = 0;
  int statusImmuneTurns = 0;
  int attributeResistTurns = 0;
  int cantMoveTurns = 0;
  int reflectAllTurns = 0;
  int damageGuardCount = 0;
  bool isDoubleAction = 0; // 2回行動
  bool isDefending = false;
  struct PopupData {
    int value;      // 数値
    bool isCritical; // 会心かどうか
  };
  std::vector<PopupData> popupValues; // int から PopupData に変更
  int getHp(){return HP;}
  void setHp(int val){ HP = val;}
  int getMaxHp(){return MaxHP;}
  int getTp(){return TP;}
  int getMaxTp(){return MaxTP;}
  int getAttack() {
    float finalValue = (float)this->Attack; // 基礎ATK
    // 武器の補正
    if (equipWeaponId != -1) {
      Item weapon = createItemById(equipWeaponId);
      finalValue += weapon.value; // 武器の攻撃力を足す
    }
    // アクセの補正 (例: パワーリング ID 500)
    if (equipAccId == 500) finalValue += 10;
    if (equipAccId == 599) finalValue += 500;
    // バフを適用 (例: 1.5倍 or 2.0倍)
    if (atkBuffTurns > 0) {
      finalValue *= atkBuffRate;
    }
    // デバフを適用 (例: 0.7倍 or 0.5倍)
    if (atkDebuffTurns > 0) {
      finalValue *= atkDebuffRate;
    }
    
    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }
  int getDefense() {
    float finalValue = (float)this->Defense; // 基礎DEF
    // 防具の補正
    if (equipArmorId != -1) {
      Item armor = createItemById(equipArmorId);
      finalValue += armor.value;
    }
    if (equipAccId == 501) finalValue += 10;  // ガードリング
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (defBuffTurns > 0) {
        finalValue *= defBuffRate;
    }
    if (defDebuffTurns > 0) {
        finalValue *= defDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }
  int getSpeed() {
    float finalValue = (float)this->Speed; // 基礎SPD
    // アクセ補正
    if (equipAccId == 502) finalValue += 10;  // クイックブーツ
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (spdBuffTurns > 0) {
        finalValue *= spdBuffRate;
    }
    if (spdDebuffTurns > 0) {
        finalValue *= spdDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }
  int getSense() {
    float finalValue = (float)this->Sense; // 基礎SEN
    // アクセ補正
    if (equipAccId == 503) finalValue += 10;  // スナイパーレンズ
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (senBuffTurns > 0) {
        finalValue *= senBuffRate;
    }
    if (senDebuffTurns > 0) {
        finalValue *= senDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }
  int getLuck() {
    float finalValue = (float)this->Luck; // 基礎LCK
    // アクセ補正
    if (equipAccId == 504) finalValue += 50;  // ラッキーコイン
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア

    if (lckBuffTurns > 0) {
        finalValue *= lckBuffRate;
    }
    if (lckDebuffTurns > 0) {
        finalValue *= lckDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }
  int getLevel(){return Level;}
  int getXP(){ return currentXP; }
  int getNextXP(){ return nextLevelXP; }
  int getWho(){ return who; }
  const char* getname(){return name;}
  int getPoisonTurns() const { return poisonTurns; }
  int getParalyzedTurns() const { return paralyzedTurns; }
  int getSilencedTurns() const { return silencedTurns; }
  int getConfusedTurns() const { return confusedTurns; }
  int normalizeWho(int w) {
    if (w >= 0 && w <= 37) return w + 39;
    return w; 
  }
  void updateName() {
    int wd = normalizeWho(who);
    switch (wd) {
      case 38: name = "調律師"; break;
      case 39: name = "ストライカー"; break;
      case 40: name = "ガーディアン"; break;
      case 41: name = "クリッパー"; break;
      case 42: name = "メンダー"; break;
      case 43: name = "ジャマー"; break;
      // 1段階目
      case 44: name = "ギア・ソルジャー"; break;
      case 45: name = "フォートレス"; break;
      case 46: name = "ブレード・ダンサー"; break;
      case 47: name = "ハーモニスト"; break;
      case 48: name = "ディスアレンジャー"; break;
      // 2段階目
      case 49: name = "ギア・ナイト"; break;
      case 50: name = "キャッスル・ウォール"; break;
      case 51: name = "サイクロン・ダンサー"; break;
      case 52: name = "コンダクター"; break;
      case 53: name = "カオス・ジャマー"; break;
      // 3段階目
      case 54: name = "ヴァンガード"; break;
      case 55: name = "スワッシュバックラー"; break;
      case 56: name = "バスティオン"; break;
      case 57: name = "イージス"; break;
      case 58: name = "テンペスト"; break;
      case 59: name = "マエストロ"; break;
      case 60: name = "ディスコード"; break;
      // 4段階目
      case 61: name = "オーバーロード"; break;
      case 62: name = "デストロイヤー"; break;
      case 63: name = "ゼロ・ブレード"; break;
      case 64: name = "ヴォイド・ストーカー"; break;
      case 65: name = "アダマンタイト"; break;
      case 66: name = "インヴィンシブル"; break;
      case 67: name = "ディヴァイン・シールド"; break;
      case 68: name = "リジェネレーター"; break;
      case 69: name = "シルフィード"; break;
      case 70: name = "アクセラレーター"; break;
      case 71: name = "アーク・ハーモニスト"; break;
      case 72: name = "シンフォニア"; break;
      case 73: name = "パンデモニウム"; break;
      case 74: name = "ジョーカー"; break;
      // 特殊
      case 75: name = "はぐれノイズ"; break;
      case 76: name = "ノイズ・キング"; break;
      case 80: name = "スプラウト・ギア"; break;
      case 81: name = "ソレイユ"; break;
      case 82: name = "ハーベスト・スティンガー"; break;
      case 83: name = "雪影"; break;
      case 84: name = "デウス・エクス・マキナ"; break;

      default: name = "???"; break;
    }
  }
  enum EvolutionTriggerType {
    TRIGGER_LEVEL, // レベルアップ
    TRIGGER_ITEM   // アイテム使用
  };
  float getEvasionBonus() {
    float bonus = 1.0; // 通常 1.0倍

    if (evaBuffTurns > 0) {
        bonus *= evaBuffRate;
    }
    
    return bonus;
  }
  struct SkillLearningRule {
    int who_id; // 対象のID (例: 39)
    int level;  // 習得レベル (例: 10)
    int skill_id; // 習得する旋律のID (例: 101)
  };
  static const SkillLearningRule skillLearningTable[];
  static const int skillLearningRuleCount;

  struct EvolutionRule {
    int before_who_id;
    EvolutionTriggerType trigger_type;
    int trigger_value; 
    int after_who_id;
  };

  static const EvolutionRule evolutionTable[];
  static const int evolutionRuleCount;
  std::vector<Skill>& getLearnedSkills() {
    return learnedSkills;
  }

  int decide_MaxHP(int l, int who){
    if(who == 0) return l * 3 + 12; // 0:ストライカー (並)
    if(who == 1) return l * 5 + 15; // 1:ガーディアン (高い)
    if(who == 2) return l * 2 + 8;  // 2:クリッパー (低い)
    if(who == 3) return l * 3 + 12; // 3:メンダー (並)
    if(who == 4) return l * 2 + 10; // 4:ジャマー (低い)

    // 1段階目進化
    if(who == 5) return l * 4 + 15; // 5:ギア・ソルジャー (並+)
    if(who == 6) return l * 6 + 20; // 6:フォートレス (高い+)
    if(who == 7) return l * 3 + 10; // 7:ブレード・ダンサー (低い+)
    if(who == 8) return l * 4 + 14; // 8:ハーモニスト (並+)
    if(who == 9) return l * 3 + 12; // 9:ディスアレンジャー (低い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 5 + 20; // 10:ギア・ナイト (並++)
    if(who == 11) return l * 7 + 25; // 11:キャッスル・ウォール (高い++)
    if(who == 12) return l * 4 + 12; // 12:サイクロン・ダンサー (低い++)
    if(who == 13) return l * 5 + 16; // 13:コンダクター (並++)
    if(who == 14) return l * 4 + 14; // 14:カオス・ジャマー (低い++)
    // 3段階目進化 (15-21)
    if(who == 15) return l * 6 + 25; // 15:ヴァンガード (純ATK)
    if(who == 16) return l * 5 + 20; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 17) return l * 8 + 30; // 17:バスティオン (純DEF)
    if(who == 18) return l * 10 + 40; // 18:イージス (DEF+HP)
    if(who == 19) return l * 5 + 15; // 19:テンペスト (SPD)
    if(who == 20) return l * 6 + 18; // 20:マエストロ (SEN)
    if(who == 21) return l * 5 + 16; // 21:ディスコード (LCK)
    // 4段階目 (22-35)
    if(who == 22) return l * 8 + 30;  // 22:オーバーロード (バランス)
    if(who == 23) return l * 6 + 25;  // 23:デストロイヤー (ATK特化)
    if(who == 24) return l * 6 + 22;  // 24:ゼロ・ブレード (ATK/SPD)
    if(who == 25) return l * 5 + 20;  // 25:ヴォイド・ストーカー (SPD/LCK)
    if(who == 26) return l * 9 + 35;  // 26:アダマンタイト (純DEF)
    if(who == 27) return l * 12 + 50; // 27:インヴィンシブル (DEF+HP)
    if(who == 28) return l * 10 + 40; // 28:ディヴァイン・シールド (DEF/SEN)
    if(who == 29) return l * 12 + 50; // 29:リジェネレーター (HP/SEN)
    if(who == 30) return l * 6 + 18;  // 30:シルフィード (SPD/LCK)
    if(who == 31) return l * 6 + 20;  // 31:アクセラレーター (SPD/ATK)
    if(who == 32) return l * 7 + 20;  // 32:アーク・ハーモニスト (純SEN)
    if(who == 33) return l * 8 + 22;  // 33:シンフォニア (万能SEN)
    if(who == 34) return l * 7 + 18;  // 34:パンデモニウム (LCK/SEN)
    if(who == 35) return l * 6 + 18;  // 35:ジョーカー (LCK/SPD)
    // 特殊 (36-37)
    if(who == 36) return 8;  // 36:はぐれノイズ (HP固定)
    if(who == 37) return 12; // 37:ノイズ・キング (HP固定)

    //主人公
    if(who == 38) return l * 3 + 20;

    //仲間
    if(who == 39) return l * 3 + 12;
    if(who == 40) return l * 5 + 15;
    if(who == 41) return l * 2 + 8;
    if(who == 42) return l * 3 + 12;
    if(who == 43) return l * 2 + 10;
    //1段階進化
    if(who == 44) return l * 4 + 15;
    if(who == 45) return l * 6 + 20;
    if(who == 46) return l * 3 + 10;
    if(who == 47) return l * 4 + 14;
    if(who == 48) return l * 3 + 12;
    // 2段階目進化 (10-14)
    if(who == 49) return l * 5 + 20; // 10:ギア・ナイト (並++)
    if(who == 50) return l * 7 + 25; // 11:キャッスル・ウォール (高い++)
    if(who == 51) return l * 4 + 12; // 12:サイクロン・ダンサー (低い++)
    if(who == 52) return l * 5 + 16; // 13:コンダクター (並++)
    if(who == 53) return l * 4 + 14; // 14:カオス・ジャマー (低い++)
    // 3段階目進化 (15-21)
    if(who == 54) return l * 6 + 25; // 15:ヴァンガード (純ATK)
    if(who == 55) return l * 5 + 20; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 56) return l * 8 + 30; // 17:バスティオン (純DEF)
    if(who == 57) return l * 10 + 40; // 18:イージス (DEF+HP)
    if(who == 58) return l * 5 + 15; // 19:テンペスト (SPD)
    if(who == 59) return l * 6 + 18; // 20:マエストロ (SEN)
    if(who == 60) return l * 5 + 16; // 21:ディスコード (LCK)
    // 4段階目 (22-35)
    if(who == 61) return l * 8 + 30;  // 22:オーバーロード (バランス)
    if(who == 62) return l * 6 + 25;  // 23:デストロイヤー (ATK特化)
    if(who == 63) return l * 6 + 22;  // 24:ゼロ・ブレード (ATK/SPD)
    if(who == 64) return l * 5 + 20;  // 25:ヴォイド・ストーカー (SPD/LCK)
    if(who == 65) return l * 9 + 35;  // 26:アダマンタイト (純DEF)
    if(who == 66) return l * 12 + 50; // 27:インヴィンシブル (DEF+HP)
    if(who == 67) return l * 10 + 40; // 28:ディヴァイン・シールド (DEF/SEN)
    if(who == 68) return l * 12 + 50; // 29:リジェネレーター (HP/SEN)
    if(who == 69) return l * 6 + 18;  // 30:シルフィード (SPD/LCK)
    if(who == 70) return l * 6 + 20;  // 31:アクセラレーター (SPD/ATK)
    if(who == 71) return l * 7 + 20;  // 32:アーク・ハーモニスト (純SEN)
    if(who == 72) return l * 8 + 22;  // 33:シンフォニア (万能SEN)
    if(who == 73) return l * 7 + 18;  // 34:パンデモニウム (LCK/SEN)
    if(who == 74) return l * 6 + 18;  // 35:ジョーカー (LCK/SPD)
    if(who == 80) return 500;
    if(who == 81) return 1200;
    if(who == 82) return 3000;
    if(who == 83) return 8000;
    if(who == 84) return 20000;
    // 特殊 (36-37)
    if(who == 36) return l + 8;  // 36:はぐれノイズ (HP固定)
    if(who == 37) return l * 2 + 12; // 37:ノイズ・キング (HP固定)
    return 0;
  }
  int decide_MaxTP(int l, int who){
    if(who == 0) return l * 1 + 5;  // 0:ストライカー (低い)
    if(who == 1) return l * 1 + 5;  // 1:ガーディアン (低い)
    if(who == 2) return l * 2 + 10; // 2:クリッパー (並)
    if(who == 3) return l * 5 + 15; // 3:メンダー (高い)
    if(who == 4) return l * 4 + 10; // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 1 + 6;  // 5:ギア・ソルジャー (低い)
    if(who == 6) return l * 1 + 6;  // 6:フォートレス (低い)
    if(who == 7) return l * 2 + 12; // 7:ブレード・ダンサー (並)
    if(who == 8) return l * 6 + 18; // 8:ハーモニスト (高い+)
    if(who == 9) return l * 5 + 12; // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 7;  // 10:ギア・ナイト (低い)
    if(who == 11) return l * 1 + 7;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 2 + 15; // 12:サイクロン・ダンサー (並)
    if(who == 13) return l * 7 + 22; // 13:コンダクター (高い++)
    if(who == 14) return l * 6 + 15; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 1 + 8;  // 15:ヴァンガード
    if(who == 16) return l * 2 + 10; // 16:スワッシュバックラー
    if(who == 17) return l * 1 + 8;  // 17:バスティオン
    if(who == 18) return l * 4 + 20; // 18:イージス
    if(who == 19) return l * 3 + 18; // 19:テンペスト
    if(who == 20) return l * 9 + 30; // 20:マエストロ
    if(who == 21) return l * 8 + 25; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 2 + 10;  // 22:オーバーロード
    if(who == 23) return l * 1 + 8;   // 23:デストロイヤー
    if(who == 24) return l * 3 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 3 + 15;  // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 10;  // 26:アダマンタイト
    if(who == 27) return l * 2 + 10;  // 27:インヴィンシブル
    if(who == 28) return l * 6 + 25;  // 28:ディヴァイン・シールド
    if(who == 29) return l * 7 + 30;  // 29:リジェネレーター
    if(who == 30) return l * 4 + 20;  // 30:シルフィード
    if(who == 31) return l * 3 + 18;  // 31:アクセラレーター
    if(who == 32) return l * 12 + 40; // 32:アーク・ハーモニスト
    if(who == 33) return l * 10 + 35; // 33:シンフォニア
    if(who == 34) return l * 11 + 30; // 34:パンデモニウム
    if(who == 35) return l * 10 + 28; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return 0; // 36:はぐれノイズ (TPなし)
    if(who == 37) return 0; // 37:ノイズ・キング (TPなし)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 5;
    if(who == 40) return l * 1 + 5;  // 1:ガーディアン (低い)
    if(who == 41) return l * 2 + 10; // 2:クリッパー (並)
    if(who == 42) return l * 5 + 15; // 3:メンダー (高い)
    if(who == 43) return l * 4 + 10; // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 1 + 6;  // 5:ギア・ソルジャー (低い)
    if(who == 45) return l * 1 + 6;  // 6:フォートレス (低い)
    if(who == 46) return l * 2 + 12; // 7:ブレード・ダンサー (並)
    if(who == 47) return l * 6 + 18; // 8:ハーモニスト (高い+)
    if(who == 48) return l * 5 + 12; // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 7;  // 10:ギア・ナイト (低い)
    if(who == 50) return l * 1 + 7;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 2 + 15; // 12:サイクロン・ダンサー (並)
    if(who == 52) return l * 7 + 22; // 13:コンダクター (高い++)
    if(who == 53) return l * 6 + 15; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 1 + 8;  // 15:ヴァンガード
    if(who == 55) return l * 2 + 10; // 16:スワッシュバックラー
    if(who == 56) return l * 1 + 8;  // 17:バスティオン
    if(who == 57) return l * 4 + 20; // 18:イージス
    if(who == 58) return l * 3 + 18; // 19:テンペスト
    if(who == 59) return l * 9 + 30; // 20:マエストロ
    if(who == 60) return l * 8 + 25; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 2 + 10;  // 22:オーバーロード
    if(who == 62) return l * 1 + 8;   // 23:デストロイヤー
    if(who == 63) return l * 3 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 3 + 15;  // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 10;  // 26:アダマンタイト
    if(who == 66) return l * 2 + 10;  // 27:インヴィンシブル
    if(who == 67) return l * 6 + 25;  // 28:ディヴァイン・シールド
    if(who == 68) return l * 7 + 30;  // 29:リジェネレーター
    if(who == 69) return l * 4 + 20;  // 30:シルフィード
    if(who == 70) return l * 3 + 18;  // 31:アクセラレーター
    if(who == 71) return l * 12 + 40; // 32:アーク・ハーモニスト
    if(who == 72) return l * 10 + 35; // 33:シンフォニア
    if(who == 73) return l * 11 + 30; // 34:パンデモニウム
    if(who == 74) return l * 10 + 28; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return 0; // 36:はぐれノイズ (TPなし)
    if(who == 76) return 0; // 37:ノイズ・キング (TPなし)
    return 0;
  }
  int decide_Attack(int l, int who){
    if(who == 0) return l * 5 + 5;  // 0:ストライカー (高い)
    if(who == 1) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 2) return l * 3 + 3;  // 2:クリッパー (並)
    if(who == 3) return l * 1 + 2;  // 3:メンダー (最低)
    if(who == 4) return l * 1 + 2;  // 4:ジャマー (最低)
    // 1段階目進化
    if(who == 5) return l * 6 + 8;  // 5:ギア・ソルジャー (高い+)
    if(who == 6) return l * 2 + 4;  // 6:フォートレス (低い)
    if(who == 7) return l * 4 + 4;  // 7:ブレード・ダンサー (並+)
    if(who == 8) return l * 1 + 2;  // 8:ハーモニスト (最低)
    if(who == 9) return l * 1 + 2;  // 9:ディスアレンジャー (最低)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 7 + 12; // 10:ギア・ナイト (高い++)
    if(who == 11) return l * 2 + 5;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 5 + 6;  // 12:サイクロン・ダンサー (並++)
    if(who == 13) return l * 1 + 2;  // 13:コンダクター (最低)
    if(who == 14) return l * 1 + 2;  // 14:カオス・ジャマー (最低)
    // 3段階目 (15-21)
    if(who == 15) return l * 9 + 15; // 15:ヴァンガード (純ATK)
    if(who == 16) return l * 8 + 13; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 17) return l * 2 + 6;  // 17:バスティオン
    if(who == 18) return l * 2 + 6;  // 18:イージス
    if(who == 19) return l * 6 + 8;  // 19:テンペスト
    if(who == 20) return l * 1 + 2;  // 20:マエストロ
    if(who == 21) return l * 1 + 2;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 10 + 20; // 22:オーバーロード
    if(who == 23) return l * 12 + 25; // 23:デストロイヤー
    if(who == 24) return l * 10 + 15; // 24:ゼロ・ブレード
    if(who == 25) return l * 8 + 10;  // 25:ヴォイド・ストーカー
    if(who == 26) return l * 3 + 8;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 8;   // 27:インヴィンシブル
    if(who == 28) return l * 4 + 8;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 3 + 7;   // 29:リジェネレーター
    if(who == 30) return l * 7 + 10;  // 30:シルフィード
    if(who == 31) return l * 9 + 12;  // 31:アクセラレーター
    if(who == 32) return l * 1 + 2;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 5 + 5;   // 33:シンフォニア
    if(who == 34) return l * 2 + 3;   // 34:パンデモニウム
    if(who == 35) return l * 6 + 6;   // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 1 + 1; // 36:はぐれノイズ (攻撃しない)
    if(who == 37) return l * 1 + 1; // 37:ノイズ・キング (攻撃しない)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 5 + 5;
    if(who == 40) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 41) return l * 3 + 3;  // 2:クリッパー (並)
    if(who == 42) return l * 1 + 2;  // 3:メンダー (最低)
    if(who == 43) return l * 1 + 2;  // 4:ジャマー (最低)
    // 1段階目進化
    if(who == 44) return l * 6 + 8;  // 5:ギア・ソルジャー (高い+)
    if(who == 45) return l * 2 + 4;  // 6:フォートレス (低い)
    if(who == 46) return l * 4 + 4;  // 7:ブレード・ダンサー (並+)
    if(who == 47) return l * 1 + 2;  // 8:ハーモニスト (最低)
    if(who == 48) return l * 1 + 2;  // 9:ディスアレンジャー (最低)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 7 + 12; // 10:ギア・ナイト (高い++)
    if(who == 50) return l * 2 + 5;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 5 + 6;  // 12:サイクロン・ダンサー (並++)
    if(who == 52) return l * 1 + 2;  // 13:コンダクター (最低)
    if(who == 53) return l * 1 + 2;  // 14:カオス・ジャマー (最低)
    // 3段階目 (15-21)
    if(who == 54) return l * 9 + 15; // 15:ヴァンガード (純ATK)
    if(who == 55) return l * 8 + 13; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 56) return l * 2 + 6;  // 17:バスティオン
    if(who == 57) return l * 2 + 6;  // 18:イージス
    if(who == 58) return l * 6 + 8;  // 19:テンペスト
    if(who == 59) return l * 1 + 2;  // 20:マエストロ
    if(who == 60) return l * 1 + 2;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 10 + 20; // 22:オーバーロード
    if(who == 62) return l * 12 + 25; // 23:デストロイヤー
    if(who == 63) return l * 10 + 15; // 24:ゼロ・ブレード
    if(who == 64) return l * 8 + 10;  // 25:ヴォイド・ストーカー
    if(who == 65) return l * 3 + 8;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 8;   // 27:インヴィンシブル
    if(who == 67) return l * 4 + 8;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 3 + 7;   // 29:リジェネレーター
    if(who == 69) return l * 7 + 10;  // 30:シルフィード
    if(who == 70) return l * 9 + 12;  // 31:アクセラレーター
    if(who == 71) return l * 1 + 2;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 5 + 5;   // 33:シンフォニア
    if(who == 73) return l * 2 + 3;   // 34:パンデモニウム
    if(who == 74) return l * 6 + 6;   // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 1 + 1; // 36:はぐれノイズ (攻撃しない)
    if(who == 76) return l * 1 + 1; // 37:ノイズ・キング (攻撃しない)
    
    return 0;
  }
  int decide_Defense(int l, int who){
    if(who == 0) return l * 1 + 3;  // 0:ストライカー (低い)
    if(who == 1) return l * 5 + 5;  // 1:ガーディアン (高い)
    if(who == 2) return l * 1 + 3;  // 2:クリッパー (低い)
    if(who == 3) return l * 3 + 5;  // 3:メンダー (並)
    if(who == 4) return l * 3 + 5;  // 4:ジャマー (並)
    // 1段階目進化
    if(who == 5) return l * 1 + 5;  // 5:ギア・ソルジャー (低い+)
    if(who == 6) return l * 6 + 8;  // 6:フォートレス (高い+)
    if(who == 7) return l * 1 + 4;  // 7:ブレード・ダンサー (低い+)
    if(who == 8) return l * 3 + 7;  // 8:ハーモニスト (並+)
    if(who == 9) return l * 3 + 6;  // 9:ディスアレンジャー (並)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 6;  // 10:ギア・ナイト (低い+)
    if(who == 11) return l * 7 + 12; // 11:キャッスル・ウォール (高い++)
    if(who == 12) return l * 1 + 5;  // 12:サイクロン・ダンサー (低い+)
    if(who == 13) return l * 4 + 9;  // 13:コンダクター (並++)
    if(who == 14) return l * 4 + 8;  // 14:カオス・ジャマー (並+)
    // 3段階目 (15-21)
    if(who == 15) return l * 3 + 8;  // 15:ヴァンガード (重装甲)
    if(who == 16) return l * 1 + 6;  // 16:スワッシュバックラー (軽装)
    if(who == 17) return l * 9 + 18; // 17:バスティオン (純DEF)
    if(who == 18) return l * 8 + 15; // 18:イージス (DEF+HP)
    if(who == 19) return l * 1 + 6;  // 19:テンペスト
    if(who == 20) return l * 5 + 11; // 20:マエストロ
    if(who == 21) return l * 5 + 10; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 6 + 10;  // 22:オーバーロード
    if(who == 23) return l * 1 + 5;   // 23:デストロイヤー
    if(who == 24) return l * 2 + 7;   // 24:ゼロ・ブレード
    if(who == 25) return l * 1 + 6;   // 25:ヴォイド・ストーカー
    if(who == 26) return l * 12 + 25; // 26:アダマンタイト
    if(who == 27) return l * 10 + 20; // 27:インヴィンシブル
    if(who == 28) return l * 10 + 18; // 28:ディヴァイン・シールド
    if(who == 29) return l * 8 + 15;  // 29:リジェネレーター
    if(who == 30) return l * 3 + 8;   // 30:シルフィード
    if(who == 31) return l * 2 + 7;   // 31:アクセラレーター
    if(who == 32) return l * 6 + 12;  // 32:アーク・ハーモニスト
    if(who == 33) return l * 7 + 14;  // 33:シンフォニア
    if(who == 34) return l * 6 + 12;  // 34:パンデモニウム
    if(who == 35) return l * 5 + 10;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 500 + 200; // 36:はぐれノイズ (超防御)
    if(who == 37) return l * 700 + 300; // 37:ノイズ・キング (極防御)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 3;  // 0:ストライカー (低い)
    if(who == 40) return l * 5 + 5;  // 1:ガーディアン (高い)
    if(who == 41) return l * 1 + 3;  // 2:クリッパー (低い)
    if(who == 42) return l * 3 + 5;  // 3:メンダー (並)
    if(who == 43) return l * 3 + 5;  // 4:ジャマー (並)
    // 1段階目進化
    if(who == 44) return l * 1 + 5;  // 5:ギア・ソルジャー (低い+)
    if(who == 45) return l * 6 + 8;  // 6:フォートレス (高い+)
    if(who == 46) return l * 1 + 4;  // 7:ブレード・ダンサー (低い+)
    if(who == 47) return l * 3 + 7;  // 8:ハーモニスト (並+)
    if(who == 48) return l * 3 + 6;  // 9:ディスアレンジャー (並)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 6;  // 10:ギア・ナイト (低い+)
    if(who == 50) return l * 7 + 12; // 11:キャッスル・ウォール (高い++)
    if(who == 51) return l * 1 + 5;  // 12:サイクロン・ダンサー (低い+)
    if(who == 52) return l * 4 + 9;  // 13:コンダクター (並++)
    if(who == 53) return l * 4 + 8;  // 14:カオス・ジャマー (並+)
    // 3段階目 (15-21)
    if(who == 54) return l * 3 + 8;  // 15:ヴァンガード (重装甲)
    if(who == 55) return l * 1 + 6;  // 16:スワッシュバックラー (軽装)
    if(who == 56) return l * 9 + 18; // 17:バスティオン (純DEF)
    if(who == 57) return l * 8 + 15; // 18:イージス (DEF+HP)
    if(who == 58) return l * 1 + 6;  // 19:テンペスト
    if(who == 59) return l * 5 + 11; // 20:マエストロ
    if(who == 60) return l * 5 + 10; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 6 + 10;  // 22:オーバーロード
    if(who == 62) return l * 1 + 5;   // 23:デストロイヤー
    if(who == 63) return l * 2 + 7;   // 24:ゼロ・ブレード
    if(who == 64) return l * 1 + 6;   // 25:ヴォイド・ストーカー
    if(who == 65) return l * 12 + 25; // 26:アダマンタイト
    if(who == 66) return l * 10 + 20; // 27:インヴィンシブル
    if(who == 67) return l * 10 + 18; // 28:ディヴァイン・シールド
    if(who == 68) return l * 8 + 15;  // 29:リジェネレーター
    if(who == 69) return l * 3 + 8;   // 30:シルフィード
    if(who == 70) return l * 2 + 7;   // 31:アクセラレーター
    if(who == 71) return l * 6 + 12;  // 32:アーク・ハーモニスト
    if(who == 72) return l * 7 + 14;  // 33:シンフォニア
    if(who == 73) return l * 6 + 12;  // 34:パンデモニウム
    if(who == 74) return l * 5 + 10;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 500 + 200; // 36:はぐれノイズ (超防御)
    if(who == 76) return l * 700 + 300; // 37:ノイズ・キング (極防御)

    return 0;
  }
  int decide_Speed(int l, int who){
    if(who == 0) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 1) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 2) return l * 5 + 5;  // 2:クリッパー (高い)
    if(who == 3) return l * 2 + 5;  // 3:メンダー (低い)
    if(who == 4) return l * 4 + 3;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 3 + 5;  // 5:ギア・ソルジャー (並)
    if(who == 6) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 7) return l * 6 + 8;  // 7:ブレード・ダンサー (高い+)
    if(who == 8) return l * 2 + 5;  // 8:ハーモニスト (低い)
    if(who == 9) return l * 5 + 4;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 3 + 6;  // 10:ギア・ナイト (並)
    if(who == 11) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 12) return l * 7 + 12; // 12:サイクロン・ダンサー (高い++)
    if(who == 13) return l * 2 + 6;  // 13:コンダクター (低い)
    if(who == 14) return l * 6 + 6;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 2 + 7;  // 15:ヴァンガード (遅い)
    if(who == 16) return l * 7 + 10; // 16:スワッシュバックラー (速い)
    if(who == 17) return l * 1 + 2;  // 17:バスティオン
    if(who == 18) return l * 2 + 3;  // 18:イージス
    if(who == 19) return l * 9 + 15; // 19:テンペスト (最速)
    if(who == 20) return l * 3 + 7;  // 20:マエストロ
    if(who == 21) return l * 7 + 8;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 3 + 8;   // 22:オーバーロード
    if(who == 23) return l * 2 + 7;   // 23:デストロイヤー
    if(who == 24) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 27) return l * 1 + 2;   // 27:インヴィンシブル
    if(who == 28) return l * 3 + 5;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 2 + 4;   // 29:リジェネレーター
    if(who == 30) return l * 11 + 18; // 30:シルフィード
    if(who == 31) return l * 10 + 16; // 31:アクセラレーター
    if(who == 32) return l * 4 + 8;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 6 + 10;  // 33:シンフォニア
    if(who == 34) return l * 8 + 10;  // 34:パンデモニウム
    if(who == 35) return l * 9 + 12;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 10 + 50; // 36:はぐれノイズ (超素早さ)
    if(who == 37) return l * 12 + 60; // 37:ノイズ・キング (極素早さ)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 40) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 41) return l * 5 + 5;  // 2:クリッパー (高い)
    if(who == 42) return l * 2 + 5;  // 3:メンダー (低い)
    if(who == 43) return l * 4 + 3;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 3 + 5;  // 5:ギア・ソルジャー (並)
    if(who == 45) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 46) return l * 6 + 8;  // 7:ブレード・ダンサー (高い+)
    if(who == 47) return l * 2 + 5;  // 8:ハーモニスト (低い)
    if(who == 48) return l * 5 + 4;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 3 + 6;  // 10:ギア・ナイト (並)
    if(who == 50) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 51) return l * 7 + 12; // 12:サイクロン・ダンサー (高い++)
    if(who == 52) return l * 2 + 6;  // 13:コンダクター (低い)
    if(who == 53) return l * 6 + 6;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 2 + 7;  // 15:ヴァンガード (遅い)
    if(who == 55) return l * 7 + 10; // 16:スワッシュバックラー (速い)
    if(who == 56) return l * 1 + 2;  // 17:バスティオン
    if(who == 57) return l * 2 + 3;  // 18:イージス
    if(who == 58) return l * 9 + 15; // 19:テンペスト (最速)
    if(who == 59) return l * 3 + 7;  // 20:マエストロ
    if(who == 60) return l * 7 + 8;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 3 + 8;   // 22:オーバーロード
    if(who == 62) return l * 2 + 7;   // 23:デストロイヤー
    if(who == 63) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 66) return l * 1 + 2;   // 27:インヴィンシブル
    if(who == 67) return l * 3 + 5;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 2 + 4;   // 29:リジェネレーター
    if(who == 69) return l * 11 + 18; // 30:シルフィード
    if(who == 70) return l * 10 + 16; // 31:アクセラレーター
    if(who == 71) return l * 4 + 8;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 6 + 10;  // 33:シンフォニア
    if(who == 73) return l * 8 + 10;  // 34:パンデモニウム
    if(who == 74) return l * 9 + 12;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 100 + 500; // 36:はぐれノイズ (超素早さ)
    if(who == 76) return l * 120 + 600; // 37:ノイズ・キング (極素早さ)
    return 0;
  }
  int decide_Sense(int l, int who){
    if(who == 0) return l * 1 + 2;  // 0:ストライカー (最低)
    if(who == 1) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 2) return l * 2 + 3;  // 2:クリッパー (低い)
    if(who == 3) return l * 5 + 5;  // 3:メンダー (高い)
    if(who == 4) return l * 4 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 1 + 2;  // 5:ギア・ソルジャー (最低)
    if(who == 6) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 7) return l * 2 + 3;  // 7:ブレード・ダンサー (低い)
    if(who == 8) return l * 6 + 8;  // 8:ハーモニスト (高い+)
    if(who == 9) return l * 5 + 6;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 2;  // 10:ギア・ナイト (最低)
    if(who == 11) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 12) return l * 2 + 3;  // 12:サイクロン・ダンサー (低い)
    if(who == 13) return l * 7 + 12; // 13:コンダクター (高い++)
    if(who == 14) return l * 6 + 8;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 1 + 2;  // 15:ヴァンガード
    if(who == 16) return l * 3 + 5;  // 16:スワッシュバックラー
    if(who == 17) return l * 1 + 2;  // 17:バスティオン
    if(who == 18) return l * 5 + 10; // 18:イージス (支援)
    if(who == 19) return l * 2 + 3;  // 19:テンペスト
    if(who == 20) return l * 9 + 18; // 20:マエストロ (最高)
    if(who == 21) return l * 8 + 12; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 2 + 3;   // 22:オーバーロード
    if(who == 23) return l * 1 + 2;   // 23:デストロイヤー
    if(who == 24) return l * 4 + 6;   // 24:ゼロ・ブレード
    if(who == 25) return l * 3 + 5;   // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 5;   // 27:インヴィンシブル
    if(who == 28) return l * 8 + 15;  // 28:ディヴァイン・シールド
    if(who == 29) return l * 9 + 18;  // 29:リジェネレーター
    if(who == 30) return l * 3 + 5;   // 30:シルフィード
    if(who == 31) return l * 2 + 4;   // 31:アクセラレーター
    if(who == 32) return l * 12 + 25; // 32:アーク・ハーモニスト
    if(who == 33) return l * 10 + 20; // 33:シンフォニア
    if(who == 34) return l * 11 + 20; // 34:パンデモニウム
    if(who == 35) return l * 9 + 15;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 8 + 25; // 36:はぐれノイズ (旋律なし)
    if(who == 37) return l * 12 + 25; // 37:ノイズ・キング (旋律なし)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 2;  // 0:ストライカー (最低)
    if(who == 40) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 41) return l * 2 + 3;  // 2:クリッパー (低い)
    if(who == 42) return l * 5 + 5;  // 3:メンダー (高い)
    if(who == 43) return l * 4 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 1 + 2;  // 5:ギア・ソルジャー (最低)
    if(who == 45) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 46) return l * 2 + 3;  // 7:ブレード・ダンサー (低い)
    if(who == 47) return l * 6 + 8;  // 8:ハーモニスト (高い+)
    if(who == 48) return l * 5 + 6;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 2;  // 10:ギア・ナイト (最低)
    if(who == 50) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 51) return l * 2 + 3;  // 12:サイクロン・ダンサー (低い)
    if(who == 52) return l * 7 + 12; // 13:コンダクター (高い++)
    if(who == 53) return l * 6 + 8;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 1 + 2;  // 15:ヴァンガード
    if(who == 55) return l * 3 + 5;  // 16:スワッシュバックラー
    if(who == 56) return l * 1 + 2;  // 17:バスティオン
    if(who == 57) return l * 5 + 10; // 18:イージス (支援)
    if(who == 58) return l * 2 + 3;  // 19:テンペスト
    if(who == 59) return l * 9 + 18; // 20:マエストロ (最高)
    if(who == 60) return l * 8 + 12; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 2 + 3;   // 22:オーバーロード
    if(who == 62) return l * 1 + 2;   // 23:デストロイヤー
    if(who == 63) return l * 4 + 6;   // 24:ゼロ・ブレード
    if(who == 64) return l * 3 + 5;   // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 5;   // 27:インヴィンシブル
    if(who == 67) return l * 8 + 15;  // 28:ディヴァイン・シールド
    if(who == 68) return l * 9 + 18;  // 29:リジェネレーター
    if(who == 69) return l * 3 + 5;   // 30:シルフィード
    if(who == 70) return l * 2 + 4;   // 31:アクセラレーター
    if(who == 71) return l * 12 + 25; // 32:アーク・ハーモニスト
    if(who == 72) return l * 10 + 20; // 33:シンフォニア
    if(who == 73) return l * 11 + 20; // 34:パンデモニウム
    if(who == 74) return l * 9 + 15;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 8 + 25; // 36:はぐれノイズ (旋律なし)
    if(who == 76) return l * 12 + 25; // 37:ノイズ・キング (旋律なし)
    return 0;
  }
  int decide_Luck(int l, int who){
    if(who == 0) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 1) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 2) return l * 4 + 5;  // 2:クリッパー (高い)
    if(who == 3) return l * 3 + 3;  // 3:メンダー (並)
    if(who == 4) return l * 5 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 3 + 3;  // 5:ギア・ソルジャー (並)
    if(who == 6) return l * 2 + 3;  // 6:フォートレス (低い)
    if(who == 7) return l * 5 + 6;  // 7:ブレード・ダンサー (高い+)
    if(who == 8) return l * 3 + 3;  // 8:ハーモニスト (並)
    if(who == 9) return l * 6 + 8;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 3 + 3;  // 10:ギア・ナイト (並)
    if(who == 11) return l * 2 + 3;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 6 + 8;  // 12:サイクロン・ダンサー (高い++)
    if(who == 13) return l * 3 + 3;  // 13:コンダクター (並)
    if(who == 14) return l * 7 + 12; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 4 + 4;  // 15:ヴァンガード
    if(who == 16) return l * 7 + 10; // 16:スワッシュバックラー (クリティカル)
    if(who == 17) return l * 2 + 3;  // 17:バスティオン
    if(who == 18) return l * 4 + 5;  // 18:イージス
    if(who == 19) return l * 7 + 10; // 19:テンペスト
    if(who == 20) return l * 4 + 4;  // 20:マエストロ
    if(who == 21) return l * 9 + 15; // 21:ディスコード (最高)
    // 4段階目 (22-35)
    if(who == 22) return l * 5 + 5;   // 22:オーバーロード
    if(who == 23) return l * 4 + 4;   // 23:デストロイヤー
    if(who == 24) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 26) return l * 2 + 3;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 4;   // 27:インヴィンシブル
    if(who == 28) return l * 5 + 6;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 4 + 5;   // 29:リジェネレーター
    if(who == 30) return l * 10 + 12; // 30:シルフィード
    if(who == 31) return l * 8 + 10;  // 31:アクセラレーター
    if(who == 32) return l * 5 + 5;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 6 + 6;   // 33:シンフォニア
    if(who == 34) return l * 12 + 20; // 34:パンデモニウム
    if(who == 35) return l * 11 + 18; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 15 + 50; // 36:はぐれノイズ (高幸運)
    if(who == 37) return l * 20 + 60; // 37:ノイズ・キング (高幸運)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 40) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 41) return l * 4 + 5;  // 2:クリッパー (高い)
    if(who == 42) return l * 3 + 3;  // 3:メンダー (並)
    if(who == 43) return l * 5 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 3 + 3;  // 5:ギア・ソルジャー (並)
    if(who == 45) return l * 2 + 3;  // 6:フォートレス (低い)
    if(who == 46) return l * 5 + 6;  // 7:ブレード・ダンサー (高い+)
    if(who == 47) return l * 3 + 3;  // 8:ハーモニスト (並)
    if(who == 48) return l * 6 + 8;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 3 + 3;  // 10:ギア・ナイト (並)
    if(who == 50) return l * 2 + 3;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 6 + 8;  // 12:サイクロン・ダンサー (高い++)
    if(who == 52) return l * 3 + 3;  // 13:コンダクター (並)
    if(who == 53) return l * 7 + 12; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 4 + 4;  // 15:ヴァンガード
    if(who == 55) return l * 7 + 10; // 16:スワッシュバックラー (クリティカル)
    if(who == 56) return l * 2 + 3;  // 17:バスティオン
    if(who == 57) return l * 4 + 5;  // 18:イージス
    if(who == 58) return l * 7 + 10; // 19:テンペスト
    if(who == 59) return l * 4 + 4;  // 20:マエストロ
    if(who == 60) return l * 9 + 15; // 21:ディスコード (最高)
    // 4段階目 (22-35)
    if(who == 61) return l * 5 + 5;   // 22:オーバーロード
    if(who == 62) return l * 4 + 4;   // 23:デストロイヤー
    if(who == 63) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 65) return l * 2 + 3;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 4;   // 27:インヴィンシブル
    if(who == 67) return l * 5 + 6;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 4 + 5;   // 29:リジェネレーター
    if(who == 69) return l * 10 + 12; // 30:シルフィード
    if(who == 70) return l * 8 + 10;  // 31:アクセラレーター
    if(who == 71) return l * 5 + 5;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 6 + 6;   // 33:シンフォニア
    if(who == 73) return l * 12 + 20; // 34:パンデモニウム
    if(who == 74) return l * 11 + 18; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 15 + 50; // 36:はぐれノイズ (高幸運)
    if(who == 76) return l * 20 + 60; // 37:ノイズ・キング (高幸運)
    return 0;
  }
  void setBossStats(int hp, int tp, int atk, int def, int spd, int sen, int lck) {
    MaxHP = hp;
    HP = hp;
    originalMaxHP = hp;
    MaxTP = tp;
    TP = tp;
    Attack = atk;
    Defense = def;
    Speed = spd;
    Sense = sen;
    Luck = lck;
  }
  void forceLearnSkill(int skillId) {
    learnSkill(Skill(skillId));
  }
  Status(int level, int w){
    Level = level;
    who = w;
    MaxHP = decide_MaxHP(level, w);
    originalMaxHP = MaxHP;
    HP = MaxHP;
    //tune point
    MaxTP = decide_MaxTP(level, w);
    TP = MaxTP;
    //物理攻撃に関係
    Attack = decide_Attack(level, w);
    //防御に関係
    Defense = decide_Defense(level, w);
    //回避率、命中率に関係
    Speed = decide_Speed(level, w);
    //旋律、ノイズ攻撃に使用
    Sense = decide_Sense(level, w);
    // ドロ率、クリ率に関係
    Luck = decide_Luck(level, w);
    currentXP = 0;
    nextLevelXP = calculateNextXP(Level);
    learnedSkills.reserve(10);
    updateName();
    equipWeaponId = -1;
    equipArmorId = -1;
    equipAccId = -1;
    isDoubleAction = false;
    coverTurns = 0;
    physDamageOneTurns = 0;
    endureTurns = 0;
    counterTurns = 0;
    autoReviveTurns = 0;
    statusImmuneTurns = 0;
    attributeResistTurns = 0;
    cantMoveTurns = 0;
    reflectAllTurns = 0;
    damageGuardCount = 0;
    tauntTurns = 0;
    untargetableTurns = 0;
    poisonTurns = 0; paralyzedTurns = 0; silencedTurns = 0; confusedTurns = 0;
    atkBuffRate = 1.0; atkBuffTurns = 0; defBuffRate = 1.0; defBuffTurns = 0;
    spdBuffRate = 1.0; spdBuffTurns = 0; senBuffRate = 1.0; senBuffTurns = 0;
    lckBuffRate = 1.0; lckBuffTurns = 0; evaBuffRate = 1.0; evaBuffTurns = 0;
    atkDebuffRate = 1.0; atkDebuffTurns = 0; defDebuffRate = 1.0; defDebuffTurns = 0;
    spdDebuffRate = 1.0; spdDebuffTurns = 0; senDebuffRate = 1.0; senDebuffTurns = 0;
    lckDebuffRate = 1.0; lckDebuffTurns = 0;
    hpRegenTurns = 0; tpRegenTurns = 0; invalidDamageTurns = 0; 
    reflectPhysicalTurns = 0; reflectMagicTurns = 0;
    // ★★★ 初期技の習得を、テーブル参照方式に変更 ★★★
    checkLevelUpSkills(0, Level); // 0レベルから現在のレベルまで
  }

  void resetBattleStats() {
    MaxHP = originalMaxHP; // HP上限を戻す
    if (HP > MaxHP) HP = MaxHP;
    
    // バフ・デバフ・状態異常の全解除
    coverTurns = 0;
    physDamageOneTurns = 0;
    endureTurns = 0;
    counterTurns = 0;
    autoReviveTurns = 0;
    statusImmuneTurns = 0;
    attributeResistTurns = 0;
    cantMoveTurns = 0;
    reflectAllTurns = 0;
    damageGuardCount = 0;
    tauntTurns = 0;
    untargetableTurns = 0;
    poisonTurns = 0; paralyzedTurns = 0; silencedTurns = 0; confusedTurns = 0;
    atkBuffRate = 1.0; atkBuffTurns = 0; defBuffRate = 1.0; defBuffTurns = 0;
    spdBuffRate = 1.0; spdBuffTurns = 0; senBuffRate = 1.0; senBuffTurns = 0;
    lckBuffRate = 1.0; lckBuffTurns = 0; evaBuffRate = 1.0; evaBuffTurns = 0;
    atkDebuffRate = 1.0; atkDebuffTurns = 0; defDebuffRate = 1.0; defDebuffTurns = 0;
    spdDebuffRate = 1.0; spdDebuffTurns = 0; senDebuffRate = 1.0; senDebuffTurns = 0;
    lckDebuffRate = 1.0; lckDebuffTurns = 0;
    hpRegenTurns = 0; tpRegenTurns = 0; invalidDamageTurns = 0; 
    reflectPhysicalTurns = 0; reflectMagicTurns = 0;
    clearPopups();
  }
  // ★ MaxHP増加処理 (ID 62)
  void boostMaxHP() {
    if (MaxHP == originalMaxHP) { // 重ね掛け防止
      MaxHP = (int)(MaxHP * 1.5);
      HP = (int)(HP * 1.5); // 現在HPも割合で増やす
    }
  }

  int takedamage(
      int damage,
      Skill::Category damageCategory = Skill::Category::OTHER
  ){
    // 1. ターン制無敵 (ID 63)
    if (invalidDamageTurns > 0) {
        return 0; // ダメージ0で終了 (回数は減らない)
    }
    // 2. 回数制バリア (ID 55, 64)
    if (damageGuardCount > 0) {
        damageGuardCount--; // ★ 1回消費
        return 0; // ダメージ0で終了
    }
    // 2. アダマン・ボディ
    // 物理ダメージだけを1にする
    else if (physDamageOneTurns > 0 &&
            damageCategory == Skill::Category::PHYSICAL)
    {
        if (damage > 1) {
            damage = 1;
        }
    }
    
    // 3. ラストスタンド (HP1で耐える)
    if (endureTurns > 0 && damage >= HP) {
        damage = HP - 1;
        endureTurns--; 
    }

    HP -= damage;
    if(HP < 0) HP = 0;
    
    return damage;
  }

  void ReceiveHeal(int amount) {
    if (HP <= 0) return; // 戦闘不能者は回復しない (蘇生は Revive を使う)
    
    HP += amount;
    if (HP > MaxHP) {
        HP = MaxHP;
    }
  }

  void Revive(int hpPercent) {
    if (HP > 0) return; // 既に生きている

    if (hpPercent <= 0) hpPercent = 1;
    if (hpPercent > 100) hpPercent = 100;
    
    // 最大HPの N% で復活
    HP = MaxHP * hpPercent / 100;
  }

  void CureStatus(bool cureDebuffs) {
    // 状態異常フラグをリセット
      poisonTurns = 0;
      poisonLevel = 0;
      paralyzedTurns = 0;
      silencedTurns = 0;
      confusedTurns = 0;

    // (例: スキルID 91 "リフレッシュ" の場合)
    if (cureDebuffs) {
        atkDebuffTurns = 0;
        defDebuffTurns = 0;
        spdDebuffTurns = 0;
    }
  }

  void ReceiveTP(int amount) {
    TP += amount;
    if (TP > MaxTP) {
        TP = MaxTP;
    }
    if (TP < 0) {
        TP = 0;
    }
  }

  void AddBuff(int skillId) {
    const int DEFAULT_BUFF_TURNS = 3;
    const int LONG_BUFF_TURNS = 5;

    switch (skillId) {
        // --- 主人公 ---
        case 7: // チューニング・アタック (単体 ATK)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 8: // チューニング・ガード (単体 DEF)
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 9: // アレグロ (全体 SPD)
            spdBuffRate = 1.3; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        // --- ストライカー系 ---
        case 21: // パワーチャージ (ATK 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 25: // ソードダンス (ATK/SPD 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 27: // かばう（次の自分の行動終了まで有効）
            coverTurns = 2;
            break;
        case 29: // トリックステップ (SPD/LCK 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            lckBuffRate = 1.5; lckBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 31: // 王者の波動 (味方全体 ATK/DEF 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 32: // キングス・シールド (1ターン物理・旋律反射)
          // 使用した行動の終了時に1減るため、2から開始
          reflectAllTurns = 2;
          break;
        case 34: // バーサーク (ATK 2倍 / DEF 0.5倍)
            atkBuffRate = 2.0; atkBuffTurns = LONG_BUFF_TURNS;
            defDebuffRate = 0.5; defDebuffTurns = LONG_BUFF_TURNS; // (※デバフも付く)
            break;
        case 39: // ミラーズエッジ (回避率 2倍)
            evaBuffRate = 2.0; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 43: // ダーククローク (姿を消す)
            untargetableTurns = DEFAULT_BUFF_TURNS;
            break;

        // --- ガーディアン系 ---
        case 46: // プロテクト (味方単体 DEF 1.5倍)
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 48: // ウォール (味方全体 DEF 1.2倍)
            defBuffRate = 1.2; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 49: // 挑発
            tauntTurns = DEFAULT_BUFF_TURNS;
            break;
        case 50: // リフレクト (物理攻撃反射 1回)
            reflectPhysicalTurns = 1;
            break;
        case 51: // ガーディアン魂 (DEF 2倍、SPD 0.5倍)
            defBuffRate = 2.0; defBuffTurns = LONG_BUFF_TURNS;
            spdDebuffRate = 0.5; spdDebuffTurns = LONG_BUFF_TURNS; // (※デバフも付く)
            break;
        case 52: // 鉄壁 (DEF 2.5倍)
            defBuffRate = 2.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 53: // カウンター
            counterTurns = DEFAULT_BUFF_TURNS;
            break;
        case 55: // フォースフィールド (ダメージ無効 1回)
            damageGuardCount = 1;
            break;
        case 56: // アダマン・ボディ (物理ダメージ 1)
            physDamageOneTurns = DEFAULT_BUFF_TURNS;
            break;
        case 57: // 自己修復 (HPリジェネ)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 58: // ラストスタンド (HP1で耐える 1回)
            endureTurns = 1;
            break;
        case 59: // ギガンティック・ウォール (味方全体 DEF 2倍)
            defBuffRate = 2.0; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 61: // オートリバイブ (自動蘇生 1回)
            autoReviveTurns = 99;
            break;
        case 62: // 最大HP上昇
            MaxHP = (int)(MaxHP * 1.5);
            HP = (int)(HP * 1.5);
            break;
        case 63: // 無敵 (1ターン ダメージ 0)
            // 使用直後のUpdateTurnで1減るため2から開始する
            invalidDamageTurns = 2;
            break;
        case 64: // 全体バリア (ダメージ無効 1回)
            damageGuardCount = 1;
            break;
        case 65: // 旋律反射
            reflectMagicTurns = DEFAULT_BUFF_TURNS;
            break;
        case 67: // ホーリーウォール (属性耐性UP)
            attributeResistTurns = DEFAULT_BUFF_TURNS;
            // (特殊: 属性耐性フラグ)
            break;

        // --- リジェネレーター系 ---
        case 70: // TPリジェネ
            tpRegenTurns = LONG_BUFF_TURNS;
            break;

        // --- クリッパー系 ---
        case 73: // クイックステップ (自身 SPD 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 75: // エアリアルダンス (自身 AGI 1.5倍)
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 77: // エアステップ (味方単体 SPD/AGI 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 79: // ラピッドムーブ (味方全体 SPD 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 81: // ウインドウォール (味方全体 回避率UP 1.5倍)
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 83: // 精霊の舞 (自身 TPリジェネ)
            tpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 85: // オーバークロック (自身 2回行動)
            twoActionsTurns = 1;
            isDoubleAction = true;
            break;
        case 86: // リミットブレイク (自身 ATK/SPD 最大)
            // 3ターンの間、攻撃と素早さを 4.0倍 (強烈なバフ) にする
            atkBuffRate = 4.0; 
            atkBuffTurns = 3;
            
            spdBuffRate = 4.0; 
            spdBuffTurns = 3;
            break;

        // --- メンダー系 ---
        case 95: // オートリペア (単体 HP自動回復)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 97: // リジェネフィールド (全体 HP自動回復)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 99: // ホーリー・ウォール (状態異常無効 1回)
            statusImmuneTurns = 1;
            break;
        case 102: // TPリジェネ
            tpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 103: // オラクルフィールド (全体 TP徐々に回復)
            tpRegenTurns = LONG_BUFF_TURNS;
            break;

        default:
            break;
    }
  }
  bool checkDebuffHit(int baseChance, Status* user) {
    if (user == nullptr) return true; // ユーザー指定なしなら必中（自分への副作用など）
    
    // 基本確率 + (かけた人の運 - かけられた人の運)
    int chance = baseChance + (user->getLuck() - this->getLuck());
    
    // 最低保証5%、最大95% (完全に無効化したい場合は属性耐性などで処理)
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;

    return (random(0, 100) < chance);
  }
  bool AddDebuff(int skillId, Status* user = nullptr) {
    if (statusImmuneTurns > 0) {
      statusImmuneTurns--; // 1回消費して防ぐ
      return false; // デバフを適用せずに終了
    }
    const int DEFAULT_DEBUFF_TURNS = 3;
    int baseChance = 100;
    bool sureHit = false;
    // --- 成功率の設定 ---
    switch (skillId) {
      // 毒系
      case 105: baseChance = 80; break; // ポイズン
      case 110: baseChance = 70; break; // デッドリーポイズン
      // 麻痺系
      case 45:  baseChance = 60; break; // ヴォイド・ショック
      case 47:  baseChance = 40; break; // シールドバッシュ(追加効果は低め)
      case 116: baseChance = 50; break; // ロックダウン
      // 精神系
      case 109: baseChance = 70; break; // サイレンス
      case 113: baseChance = 50; break; // カオスフィールド(混乱)
      case 122: baseChance = 50; break; // かく乱
      // ステータスダウン系
      case 10:  baseChance = 90; break; // ラレンタンド
      case 11:  baseChance = 90; break; // ノイズ・ジャミング
      case 24:  baseChance = 80; break; // アーマーブレイク
      case 87:  baseChance = 80; break; // タービュランス
      case 104: baseChance = 85; break; // スロウ
      case 106: baseChance = 85; break; // パワーダウン
      case 107: baseChance = 85; break; // ガードダウン
      case 108: baseChance = 80; break; // エリア・スロウ
      case 112: baseChance = 80; break; // エリア・ブレイク
      case 118: baseChance = 70; break; // ワールド・ダウン
      // 必中系
      case 101: sureHit = true; break; // ディスペル
      case 117: sureHit = true; break; // バニッシュ
      // 特殊
      case 114: baseChance = 40; break; // ディザスター(全体異常)
      default: baseChance = 100; break;
    }
    if (!sureHit && !checkDebuffHit(baseChance, user)) {
      return false;
    }
    switch (skillId) {
      // --- 主人公 ---
      case 10: // ラレンタンド (単体 SPD)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 11: // ノイズ・ジャミング (全体 ATK)
        atkDebuffRate = 0.8; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      // --- ストライカー系 (バフの副作用) ---
      // (case 34 は AddBuff 側で defDebuffRate = 0.5 をセット)
      // --- ギア・ナイト ---
      case 24: // アーマーブレイク (防御 0.7倍)
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- ヴォイド・ストーカー ---
      case 45: // ヴォイド・ショック (高確率で麻痺)
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- ガーディアン系 ---
      case 47: // シールドバッシュ (確率でスタン/麻痺)
        paralyzedTurns = 1;
        break;
        
      // --- ガーディアン系 (バフの副作用) ---
      // (case 51 は AddBuff 側で spdDebuffRate = 0.5 をセット)

      // --- アクセラレーター ---
      case 87: // タービュランス (全体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- メンダー系 (敵の強化解除) ---
      case 101: // ディスペル (敵単体 強化解除)
        atkBuffTurns = 0; defBuffTurns = 0; spdBuffTurns = 0;
        senBuffTurns = 0; lckBuffTurns = 0; evaBuffTurns = 0;
        hpRegenTurns = 0; tpRegenTurns = 0;
        // (他の特殊バフもすべて解除)
        break;

      // --- ジャマー系 ---
      case 104: // スロウ (敵単体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 105: // ポイズン (毒)
        // 猛毒中なら通常毒で上書きしない
        if (poisonLevel < 2) {
            poisonLevel = 1;
            poisonTurns = DEFAULT_DEBUFF_TURNS;
        }
        break;
      case 106: // パワーダウン (敵単体 ATK 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 107: // ガードダウン (敵単体 DEF 0.7倍)
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 108: // エリア・スロウ (敵全体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 109: // サイレンス (沈黙)
        silencedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 110: // デッドリーポイズン (猛毒)
        poisonLevel = 2;
        poisonTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 112: // エリア・ブレイク (敵全体 ATK/DEF 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
          break;
      case 113: // カオスフィールド (混乱)
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 114: // ディザスター (全状態異常)
        if (poisonLevel < 2) {
            poisonLevel = 1;
            poisonTurns = DEFAULT_DEBUFF_TURNS;
        }
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        silencedTurns = DEFAULT_DEBUFF_TURNS;
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 116: // ロックダウン (行動不能/麻痺)
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 117: // バニッシュ (敵単体 強化全解除)
        // (101:ディスペル と同じ効果)
        atkBuffTurns = 0; defBuffTurns = 0; spdBuffTurns = 0;
        senBuffTurns = 0; lckBuffTurns = 0; evaBuffTurns = 0;
        hpRegenTurns = 0; tpRegenTurns = 0;
        break;
      case 118: // ワールド・ダウン (敵全体 全能力 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        senDebuffRate = 0.7; senDebuffTurns = DEFAULT_DEBUFF_TURNS;
        lckDebuffRate = 0.7; lckDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

        // --- はぐれノイズ ---
      case 122: // かく乱 (混乱)
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;

        default:
            break;
    }
    return true;
  }

  int calculateNextXP(int level) {
    return 10 + (level * level * 5);
  }

  void recalculateStats() {
    // 現在のHPが最大HPを超えるのを防ぐため、上昇分だけ加算する
    int oldMaxHP = MaxHP;
    MaxHP = decide_MaxHP(Level, who);
    originalMaxHP = MaxHP;
    HP += (MaxHP - oldMaxHP); // 上昇分を加算
    if (HP > MaxHP) HP = MaxHP; // (もし全回復させたいなら HP = MaxHP;)

    // TPや他のステータスは再計算
    MaxTP = decide_MaxTP(Level, who);
    TP = MaxTP; // TPは全回復
    Attack = decide_Attack(Level, who);
    Defense = decide_Defense(Level, who);
    Speed = decide_Speed(Level, who);
    Sense = decide_Sense(Level, who);
    Luck = decide_Luck(Level, who);
  }

  bool useTP(int cost) {
    if (TP >= cost) {
      TP -= cost;
      return true;
    }
    return false; // TPが足りない
  }

  int gainXP(int amount) {
    if (Level >= 99) {
      return 0;
    }

    currentXP += amount;

    bool leveledUp = false;
    bool evolvedDuringGain = false;

    // 一度に複数レベル上がる場合でも、
    // 1レベルずつ処理して、その都度進化判定を行う。
    while (currentXP >= nextLevelXP &&
          Level < 99)
    {
      int previousLevel = Level;

      currentXP -= nextLevelXP;
      Level++;

      nextLevelXP =
          calculateNextXP(Level);

      leveledUp = true;

      // -------------------------------------------------
      // このレベルで発生する自動進化を処理
      // -------------------------------------------------
      bool evolvedAtThisLevel = false;

      while (true) {
        bool evolutionFound = false;

        for (int i = 0;
            i < evolutionRuleCount;
            i++)
        {
          const EvolutionRule& rule =
              evolutionTable[i];

          if (rule.before_who_id == this->who &&
              rule.trigger_type == TRIGGER_LEVEL &&
              this->Level >= rule.trigger_value)
          {
            evolve(rule.after_who_id);

            evolvedDuringGain = true;
            evolvedAtThisLevel = true;
            evolutionFound = true;

            // who が変わったので、
            // 新しい who でも進化条件があるか再確認する
            break;
          }
        }

        if (!evolutionFound) {
          break;
        }
      }

      // evolve() 内ではステータス再計算と
      // スキル習得処理が行われるので、
      // 進化しなかった場合だけここで処理する。
      if (!evolvedAtThisLevel) {
        recalculateStats();

        checkLevelUpSkills(
            previousLevel,
            Level
        );
      }
    }

    if (evolvedDuringGain) {
      return 2; // レベルアップ中に進化した
    }

    if (leveledUp) {
      return 1; // レベルアップのみ
    }

    return 0; // 変化なし
  }

  int caclatedamage(int Attack, int Defense, int ASpeed, int DSpeed, int ALuck, int DLuck, bool &retIsCrit){
    // 回避計算
    int baseHitRate = 95;
    int hitRate = baseHitRate + (ASpeed - DSpeed);
    if(hitRate > 100) hitRate = 100;
    else if(hitRate < 5) hitRate = 5;
    
    bool isHit = (random(0, 100) < hitRate);
    if(!isHit) {
        retIsCrit = false; // ミス
        return 0;
    }

    // クリ計算
    int basecriticalRate = 5;
    int criticalRate = basecriticalRate + (ALuck - DLuck);
    if(criticalRate > 100) criticalRate = 100;
    else if(criticalRate < 0) criticalRate = 0;
    
    // ★ 計算結果を引数の変数に書き込む
    retIsCrit = (random(0, 100) < criticalRate);

    // ダメージ計算 (前回修正した改良版の式)
    float effectiveDef = (float)Defense * 0.5;
    if (retIsCrit) effectiveDef *= 0.5; // 貫通

    int damage = (int)((float)Attack - effectiveDef);

    if (retIsCrit) damage = damage * 1.5; // 倍率

    float variance = (float)random(90, 111) / 100.0;
    damage = (int)((float)damage * variance);

    int minDamage = Attack / 20; 
    if (minDamage < 1) minDamage = 1;
    if (damage < minDamage) damage = minDamage;
    damage /= 4;
    return damage;
  }

  int caclulateSkillDamage(Status* target, Skill& skill, bool &retIsCrit) {
    // --- 0. 命中判定 (ここが抜けていました) ---
    // 回復・バフ・デバフ・蘇生・治療は必中とする
    // 攻撃系(DAMAGE, MULTI_HIT, SPECIAL)のみ判定を行う
    if (skill.type == Skill::EffectType::DAMAGE || 
      skill.type == Skill::EffectType::MULTI_HIT_DAMAGE ||
      skill.type == Skill::EffectType::SPECIAL) 
    {
      bool isSureHit = false;

      // 回避無視スキル
      if (skill.id == 80) { // インビジブルエッジ
        isSureHit = true;
      }
      // 魔法(旋律)攻撃も基本必中とするならここに追加(今回は物理と同じ扱いで計算)

      if (!isSureHit) {
        // 基本命中率
        int hitRate =
            95 +
            (this->getSpeed() - target->getSpeed());

        // まず通常状態での「回避率」を求める
        if (hitRate > 100) hitRate = 100;
        if (hitRate < 20) hitRate = 20;

        int missRate = 100 - hitRate;

        // 対象の回避率バフを反映
        // 例：回避率2倍なら、通常5%回避 → 10%回避
        float evasionBonus =
            target->getEvasionBonus();

        missRate =
            static_cast<int>(
                missRate * evasionBonus
            );

        hitRate = 100 - missRate;

        // 最終的な命中率も20～100%に制限
        if (hitRate > 100) hitRate = 100;
        if (hitRate < 20) hitRate = 20;

        // 命中判定
        if (random(0, 100) >= hitRate) {
          return 0;
        }
      }
    }
    if ((target->getWho() == 36 || target->getWho() == 37|| target->getWho() == 75 || target->getWho() == 76) && 
        skill.dependence == Skill::StatDependence::SEN) {
      retIsCrit = false;
      return 0; // ダメージ0 (無効)
    }
    // --- 基礎ステータス決定 ---
    int offensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? this->getSense() : this->getAttack();
    int defensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? target->getSense() : target->getDefense();

    // --- 特殊技の威力補正 ---
    float skillPower = (float)skill.power;

    // ID 35: カラミティ・エンド (HPが低いほど高威力)
    if (skill.id == 35) {
        float hpRatio = (float)this->getHp() / (float)this->getMaxHp();
        // HP100%なら1倍、HP1%なら3倍など
        skillPower = skillPower * (3.0 - (hpRatio * 2.0)); 
    }
    // ID 119: エナジー・バーン (敵のTPが多いほど高威力)
    else if (skill.id == 119) {
        skillPower += target->getTp() / 2; 
    }
    // ID 115: ペイン・イーター (状態異常の敵に威力2倍)
    else if (skill.id == 115) {
        if (target->getPoisonTurns() > 0 || target->getParalyzedTurns() > 0 || 
            target->getSilencedTurns() > 0 || target->getConfusedTurns() > 0) {
            skillPower *= 2.0;
        }
    }

    // --- 会心判定 ---
    int critBonus = 0;
    // ID 28, 40: 高クリティカル
    if (skill.id == 28 || skill.id == 40) critBonus = 30; 

    int critRate = 5 + (this->getLuck() - target->getLuck()) + critBonus;
    if(critRate > 100) critRate = 100;
    if(critRate < 0) critRate = 0;
    retIsCrit = (random(0, 100) < critRate);

    // --- 防御力計算 ---
    float effectiveDef = (float)defensiveVal / 2.0;

    // ID 24, 124: 防御無視 (アーマーブレイク, キングスピア)
    if (skill.id == 24 || skill.id == 124) {
        effectiveDef = 0;
    }
    // 会心時は貫通
    else if (retIsCrit) {
        effectiveDef *= 0.5; 
    }

    // --- 最終ダメージ計算 ---
    // (power 50 を基準倍率 1.0 とする)
    float powerMultiplier = skillPower / 50.0;
    int baseDamage = (int)((float)offensiveVal * powerMultiplier);
    
    int damage = baseDamage - (int)effectiveDef;
    if (target->isDefending) {
      damage = (int)(damage * 0.7);
    }
    if (target->attributeResistTurns > 0) {
      damage = damage / 4;
    }
    // 会心ボーナス (1.5倍)
    if (retIsCrit) damage = damage * 1.5;

    // 乱数
    float variance = (float)random(90, 111) / 100.0;
    damage = (int)((float)damage * variance);
    damage /= 4;
    // 最低保証
    int minDamage = offensiveVal / 20;
    if (minDamage < 1) minDamage = 1;
    if (damage < minDamage) damage = minDamage;

    return damage;
  }

  // ★★★ 3. その他の特殊処理 ★★★
  void limitBreak() { // ID 86
      // バフを最大ターン・最大倍率で付与 (簡易実装)
      atkBuffRate = 3.0; atkBuffTurns = 3;
      spdBuffRate = 3.0; spdBuffTurns = 3;
  }

  // ★★★ 修正: 引数に bool &retIsCrit を追加 ★★★
  int calculateHealAmount(Skill& skill, bool &retIsCrit) {
    int base = this->getSense();
    float multiplier = (float)skill.power / 50.0;
    int heal = (int)((float)base * multiplier);
    
    // --- ★ 会心判定 (回復) ---
    // (回復の会心率は運の10%程度とする)
    int critRate = this->getLuck() / 10; 
    if (critRate < 5) critRate = 5; // 最低5%
    
    retIsCrit = (random(0, 100) < critRate);

    if (retIsCrit) heal = heal * 2.0; // 会心なら1.5倍回復
    // ------------------------
    
    float variance =
        (float)random(90, 111) / 100.0;

    heal =
        (int)((float)heal * variance);

    // ゲームバランス用の最終補正
    heal /= 4;

    // 最終結果として最低1は回復する
    if (heal < 1) {
      heal = 1;
    }

    return heal;
  }

  bool isPlayer() {
    return (who >= 38 && who <= 79); // GDDのID定義 (38以上は味方)
  }

  void addAttack(int amount) {
    Attack += amount;
    // (将来的に上限なども設定)
  }

  void learnSkill(const Skill& newSkill) {
    // (既に覚えていないかチェックしたほうが親切だが、今は省略)
    for (const auto& s : learnedSkills) {
      if (s.id == newSkill.id) {
        return; // すでに覚えているなら何もしない
      }
    }
    // 新しい技なら追加
    learnedSkills.push_back(newSkill);
  }
  
  void evolve(int newWhoId) {
    // 1. 自分のIDを進化先のものに変更
    int oldLevel = Level;
    this->who = newWhoId;
    updateName();
    // 2. 既存のステータス再計算関数を呼び出す
    // (recalculateStats は新しい who ID を使って
    //  decide_MaxHP などを呼び出すので、これでステータスが更新される)
    recalculateStats(); 

    // 3. 進化ボーナスとしてHP/TPを全回復
    this->HP = this->MaxHP;
    this->TP = this->MaxTP;
    checkLevelUpSkills(0, oldLevel);
  }

  void checkLevelUpSkills(int oldLevel, int newLevel) {
    
    // チェック対象のID (最初は今の自分)
    int checkID = this->who;

    // ★ 進化の系譜を遡りながら全てチェックするループ
    while (checkID != -1) {
        int normalizedCheckID = normalizeWho(checkID); // 味方IDに統一

        for (int i = 0; i < skillLearningRuleCount; i++) {
          const SkillLearningRule& rule = skillLearningTable[i];
          
          // 「チェック中のID」かつ「レベル条件を満たした」なら
          if (rule.who_id == normalizedCheckID &&
              rule.level <= newLevel &&
              (rule.level > oldLevel || oldLevel == 0)) // 範囲内、または初期化時(0)
          {
            learnSkill(Skill(rule.skill_id)); // 重複チェック付きで習得
          }
        }

        // ★ 次のループのために、進化前のIDを取得する
        checkID = getPreEvolution(checkID);
    }
  }
  void UpdateTurn() {
    // -------------------------
    // 通常バフ
    // -------------------------
    if (atkBuffTurns > 0) atkBuffTurns--;
    if (defBuffTurns > 0) defBuffTurns--;
    if (spdBuffTurns > 0) spdBuffTurns--;
    if (senBuffTurns > 0) senBuffTurns--;
    if (lckBuffTurns > 0) lckBuffTurns--;
    if (evaBuffTurns > 0) evaBuffTurns--;

    if (hpRegenTurns > 0) hpRegenTurns--;
    if (tpRegenTurns > 0) tpRegenTurns--;

    // -------------------------
    // 特殊なターン制バフ
    // -------------------------
    if (coverTurns > 0) coverTurns--;
    if (tauntTurns > 0) tauntTurns--;
    if (untargetableTurns > 0) untargetableTurns--;
    if (counterTurns > 0) counterTurns--;
    if (physDamageOneTurns > 0) physDamageOneTurns--;
    if (attributeResistTurns > 0) attributeResistTurns--;

    if (invalidDamageTurns > 0) invalidDamageTurns--;

    // 反射系は現状の仕様を維持。
    // 次のコミットで「ターン制 / 回数制」を整理する。

    // キングス・シールド：ターン制
    if (reflectAllTurns > 0) reflectAllTurns--;

    // 旋律反射：ターン制
    if (reflectMagicTurns > 0) reflectMagicTurns--;

    // -------------------------
    // デバフ
    // -------------------------
    if (atkDebuffTurns > 0) atkDebuffTurns--;
    if (defDebuffTurns > 0) defDebuffTurns--;
    if (spdDebuffTurns > 0) spdDebuffTurns--;
    if (senDebuffTurns > 0) senDebuffTurns--;
    if (lckDebuffTurns > 0) lckDebuffTurns--;

    // -------------------------
    // 状態異常
    // -------------------------
    if (poisonTurns > 0) {
      poisonTurns--;

      if (poisonTurns == 0) {
        poisonLevel = 0;
      }
    }

    if (paralyzedTurns > 0) paralyzedTurns--;
    if (silencedTurns > 0) silencedTurns--;
    if (confusedTurns > 0) confusedTurns--;
  }

  int ApplyTurnEffects() {
    int value = 0;

    if (hpRegenTurns > 0) {
      int heal = getMaxHp() * 0.1;
      if (heal < 1) heal = 1;
      ReceiveHeal(heal);
      value = -heal;
    }

    if (tpRegenTurns > 0) {
      ReceiveTP(getMaxTp() * 0.05);
    }

    if (poisonTurns > 0) {
      int poisonRate = (poisonLevel >= 2) ? 20 : 10;
      int poisonDmg = getMaxHp() * poisonRate / 100;

      if (poisonDmg < 1) poisonDmg = 1;

      if (attributeResistTurns > 0) {
        poisonDmg /= 2;
        if (poisonDmg < 1) poisonDmg = 1;
      }

      takedamage(poisonDmg);
      value = poisonDmg;
    }
    return value;
  }

  void saveTo(FsFile& file) {
    file.println(who);
    file.println(Level);
    file.println(currentXP);
    file.println(HP);
    file.println(TP);
    file.println(equipWeaponId); // ★ 追加
    file.println(equipArmorId);  // ★ 追加
    file.println(equipAccId);    // ★ 追加
    // 覚えているスキルの数とID
    file.println(learnedSkills.size());
    for (const auto& s : learnedSkills) {
      file.println(s.id);
    }
  }

  void loadFrom(FsFile& file) {
    // -------------------------
    // 保存値を読み込む
    // -------------------------
    who = file.readStringUntil('\n').toInt();
    Level = file.readStringUntil('\n').toInt();
    currentXP = file.readStringUntil('\n').toInt();

    // HP / TP は再計算で上書きされるので、一旦退避する
    int savedHP = file.readStringUntil('\n').toInt();
    int savedTP = file.readStringUntil('\n').toInt();

    equipWeaponId = file.readStringUntil('\n').toInt();
    equipArmorId = file.readStringUntil('\n').toInt();
    equipAccId = file.readStringUntil('\n').toInt();

    // -------------------------
    // レベル・種類に応じた基礎値を再構築
    // -------------------------
    updateName();

    // ロードしたLevelに合わせて次の必要XPも更新
    nextLevelXP = calculateNextXP(Level);

    recalculateStats();

    // -------------------------
    // 保存時の現在HP / TPを復元
    // -------------------------
    HP = savedHP;
    TP = savedTP;

    // 壊れた/古いセーブデータ対策
    if (HP < 0) HP = 0;
    if (HP > MaxHP) HP = MaxHP;

    if (TP < 0) TP = 0;
    if (TP > MaxTP) TP = MaxTP;

    // -------------------------
    // 習得済みスキル復元
    // -------------------------
    learnedSkills.clear();

    int skillCount = file.readStringUntil('\n').toInt();

    for (int i = 0; i < skillCount; i++) {
      int skillId = file.readStringUntil('\n').toInt();
      learnedSkills.push_back(Skill(skillId));
    }

    // レベル上は覚えているはずなのに、
    // 古いセーブに存在しないスキルがあれば補完
    checkLevelUpSkills(0, Level);
  }

  void addPopup(int value, bool isCrit = false) {
    popupValues.push_back({value, isCrit});
  }

  void clearPopups() {
    popupValues.clear();
  }
  ~Status() {
    learnedSkills.clear();
    std::vector<Skill>().swap(learnedSkills);
    
    popupValues.clear();
    std::vector<PopupData>().swap(popupValues);
  }
};

const Status::EvolutionRule Status::evolutionTable[] = {
  // --- 仲間1号機 (Attack: 39) の進化ツリー ---
  { 39, Status::TRIGGER_LEVEL, 10, 44 }, // 39(Lv10) -> 44
  { 44, Status::TRIGGER_LEVEL, 20, 49 }, // 44(Lv20) -> 49
  { 49, Status::TRIGGER_LEVEL, 30, 54 }, // 49(Lv30) -> 54
  { 49, Status::TRIGGER_ITEM,  10, 55 }, // 49(Lv40) -> 55 (Aルート)
  { 54, Status::TRIGGER_LEVEL, 50, 61 }, // 54 + アイテム10 -> 61 (Bルート)
  { 54, Status::TRIGGER_ITEM,  11, 62 }, // 54 + アイテム11 -> 62
  { 55, Status::TRIGGER_LEVEL, 50, 63 }, // 55(Lv40) -> 63 (Aルート)
  { 55, Status::TRIGGER_ITEM,  12, 64 }, // 55 + アイテム12 -> 64 (Bルート)

  // --- 仲間2号機 (Defense: 48) の進化ツリー ---
  { 40, Status::TRIGGER_LEVEL, 10, 45 }, // 48(Lv10) -> 49
  { 45, Status::TRIGGER_LEVEL, 20, 50 }, // 49(Lv20) -> 50
  { 50, Status::TRIGGER_LEVEL, 30, 56 }, // 50(Lv30) -> 51
  { 50, Status::TRIGGER_ITEM,  13, 57 }, // 51(Lv40) -> 52 (Aルート)
  { 56, Status::TRIGGER_LEVEL, 50, 65 }, // 51 + アイテム13 -> 53 (Bルート)
  { 56, Status::TRIGGER_ITEM,  14, 66 }, // 50 + アイテム14 -> 54
  { 57, Status::TRIGGER_LEVEL, 50, 67 }, // 54(Lv40) -> 55 (Aルート)
  { 57, Status::TRIGGER_ITEM,  15, 68 }, // 54 + アイテム15 -> 56 (Bルート)

  // --- 仲間3号機 (Speed: 57) の進化ツリー ---
  { 41, Status::TRIGGER_LEVEL, 10, 46 },
  { 46, Status::TRIGGER_LEVEL, 20, 51 },
  { 51, Status::TRIGGER_LEVEL, 30, 58 },
  { 58, Status::TRIGGER_LEVEL, 50, 69 }, // 60(Lv40) -> 61 (Aルート)
  { 58, Status::TRIGGER_ITEM,  16, 70 }, // 60 + アイテム16 -> 62 (Bルート)

  // --- 仲間4号機 (Heal: 63) の進化ツリー ---
  { 42, Status::TRIGGER_LEVEL, 10, 47 },
  { 47, Status::TRIGGER_LEVEL, 20, 52 },
  { 52, Status::TRIGGER_LEVEL, 30, 59 },
  { 59, Status::TRIGGER_LEVEL, 50, 71 }, // 66(Lv40) -> 67 (Aルート)
  { 59, Status::TRIGGER_ITEM,  17, 72 }, // 66 + アイテム17 -> 68 (Bルート)

  // --- 仲間5号機 (Sense: 69) の進化ツリー ---
  { 43, Status::TRIGGER_LEVEL, 10, 48 },
  { 48, Status::TRIGGER_LEVEL, 20, 53 },
  { 53, Status::TRIGGER_LEVEL, 30, 60 },
  { 60, Status::TRIGGER_LEVEL, 50, 73 }, // 72(Lv40) -> 73 (Aルート)
  { 60, Status::TRIGGER_ITEM,  18, 74 }, // 72 + アイテム18 -> 74 (Bルート)

  //特殊
  { 75, Status::TRIGGER_ITEM,   4, 76 } // 金の歯車で進化
};
// サイズも同様に定義
const int Status::evolutionRuleCount = sizeof(Status::evolutionTable) / sizeof(Status::EvolutionRule);

const Status::SkillLearningRule Status::skillLearningTable[] = {
  // --- 仲間1号機 (ストライカー系) ---
  { 39, 1, 20 }, // Lv 1: ハードヒット
  { 39, 5, 21 }, // Lv 5: パワーチャージ
  
  { 44, 10, 22 }, // Lv 10: スラッシュ
  { 44, 15, 23 }, // Lv 15: ダブルヒット
  
  { 49, 20, 24 }, // Lv 20: アーマーブレイク
  { 49, 25, 25 }, // Lv 25: ソードダンス
  
  // (3段階目分岐A: ヴァンガード)
  { 54, 35, 26 }, // Lv 30: シールドバッシュ
  { 54, 45, 27 }, // Lv 35: かばう
  
  // (3段階目分岐B: スワッシュバックラー)
  { 55, 35, 28 }, // Lv 30: ファントムエッジ
  { 55, 45, 29 }, // Lv 35: トリックステップ

  // (4段階目分岐A1: オーバーロード)
  { 61, 50, 30 }, // Lv 40: ギガブレイク
  { 61, 60, 31 }, // Lv 42: 王者の波動
  { 61, 70, 32 }, // Lv 45: キングス・シールド
  { 61, 80, 33 }, // Lv 50: デッドエンド

  // (4段階目分岐A2: デストロイヤー)
  { 62, 50, 34 }, // Lv 40: バーサーク
  { 62, 60, 35 }, // Lv 42: カラミティ・エンド
  { 62, 70, 36 }, // Lv 45: トリプルスラッシュ
  { 62, 80, 37 }, // Lv 50: デモリッシュ

  // (4段階目分岐B1: ゼロ・ブレード)
  { 63, 50, 38 }, // Lv 40: ソニックブレイド
  { 63, 60, 39 }, // Lv 42: ミラーズエッジ
  { 63, 70, 40 }, // Lv 45: 刹那
  { 63, 80, 41 }, // Lv 50: 無空

  // (4段階目分岐B2: ヴォイド・ストーカー)
  { 64, 50, 42 }, // Lv 40: アサシネイト
  { 64, 60, 43 }, // Lv 42: ダーククローク
  { 64, 70, 44 }, // Lv 45: ペインサイクル
  { 64, 80, 45 }, // Lv 50: ヴォイド・ショック

  // --- 仲間2号機 (ガーディアン系) ---
  { 40, 1, 46 }, { 40, 5, 47 },   // ガーディアン (who=48)
  { 45, 10, 48 }, { 45, 15, 49 }, // フォートレス (who=49)
  { 50, 20, 50 }, { 50, 25, 51 }, // キャッスル・ウォール (who=50)
  // (分岐A)
  { 56, 35, 52 }, { 56, 45, 53 }, // バスティオン (who=51)
  { 65, 50, 56 }, { 65, 60, 57 }, { 65, 70, 58 }, { 65, 80, 59 }, // アダマンタイト (who=52)
  { 66, 50, 60 }, { 66, 60, 61 }, { 66, 70, 62 }, { 66, 80, 63 }, // インヴィンシブル (who=53)
  // (分岐B)
  { 57, 35, 54 }, { 57, 45, 55 }, // イージス (who=54)
  { 67, 50, 64 }, { 67, 60, 65 }, { 67, 70, 66 }, { 67, 80, 67 }, // ディヴァイン・シールド (who=55)
  { 68, 50, 68 }, { 68, 60, 69 }, { 68, 70, 70 }, { 68, 80, 71 }, // リジェネレーター (who=56)

  // --- 仲間3号機 (クリッパー系) ---
  // (クリッパー who=41)
  { 41, 1, 72 },   // Lv 1: カッティング
  { 41, 5, 73 },   // Lv 5: クイックステップ

  // (ブレード・ダンサー who=46)
  { 46, 10, 74 },  // Lv 10: ツインエッジ
  { 46, 15, 75 },  // Lv 15: エアリアルダンス

  // (サイクロン・ダンサー who=51)
  { 51, 20, 76 },  // Lv 20: スパイラルエッジ
  { 51, 25, 77 },  // Lv 25: エアステップ

  // (テンペスト who=58)
  { 58, 35, 78 },  // Lv 35: ゲイルストライク
  { 58, 45, 79 },  // Lv 45: ラピッドムーブ

  // (4段階目分岐A: シルフィード who=69)
  { 69, 50, 80 },  // Lv 50: インビジブルエッジ
  { 69, 60, 81 },  // Lv 60: ウインドウォール
  { 69, 70, 82 },  // Lv 70: ストームブリンガー
  { 69, 80, 83 },  // Lv 80: 精霊の舞

  // (4段階目分岐B: アクセラレーター who=70)
  { 70, 50, 84 },  // Lv 50: マッハブレイド
  { 70, 60, 85 },  // Lv 60: オーバークロック
  { 70, 70, 86 },  // Lv 70: リミットブレイク
  { 70, 80, 87 },  // Lv 80: タービュランス

  // --- 仲間4号機 (メンダー系) ---
  // (メンダー who=42)
  { 42, 1, 88 },   // Lv 1: リペア
  { 42, 5, 89 },   // Lv 5: キュア

  // (第1進化 who=47)
  { 47, 10, 90 },  // Lv 10: リペア・プラス
  { 47, 15, 91 },  // Lv 15: リフレッシュ

  // (第2進化 who=52)
  { 52, 20, 92 },  // Lv 20: エリア・リペア
  { 52, 25, 93 },  // Lv 25: リザレクション

  // (第3進化 who=59)
  { 59, 35, 94 },  // Lv 35: フル・リペア
  { 59, 45, 95 },  // Lv 45: オートリペア

  // (4段階目分岐A: who=71)
  { 71, 50, 96 },  // Lv 50: エリア・リペア・プラス
  { 71, 60, 97 },  // Lv 60: リジェネフィールド
  { 71, 70, 98 },  // Lv 70: アセンション
  { 71, 80, 99 },  // Lv 80: ホーリー・ウォール

  // (4段階目分岐B: who=72)
  { 72, 50, 100 }, // Lv 50: TPチャージ
  { 72, 60, 101 }, // Lv 60: ディスペル
  { 72, 70, 102 }, // Lv 70: TPリジェネ
  { 72, 80, 103 }, // Lv 80: オラクルフィールド

  // (ジャマー who=43)
  { 43, 1, 104 },  // Lv 1: スロウ
  { 43, 5, 105 },  // Lv 5: ポイズン

  // (第1進化 who=48)
  { 48, 10, 106 }, // Lv 10: パワーダウン
  { 48, 15, 107 }, // Lv 15: ガードダウン

  // (第2進化 who=53)
  { 53, 20, 108 }, // Lv 20: エリア・スロウ
  { 53, 25, 109 }, // Lv 25: サイレンス

  // (第3進化 who=60)
  { 60, 35, 110 }, // Lv 35: デッドリーポイズン
  { 60, 45, 111 }, // Lv 45: TPドレイン

  // (4段階目分岐A: who=73)
  { 73, 50, 112 }, // Lv 50: エリア・ブレイク
  { 73, 60, 113 }, // Lv 60: カオスフィールド
  { 73, 70, 114 }, // Lv 70: ディザスター
  { 73, 80, 115 }, // Lv 80: ペイン・イーター

  // (4段階目分岐B: who=74)
  { 74, 50, 116 }, // Lv 50: ロックダウン
  { 74, 60, 117 }, // Lv 60: バニッシュ
  { 74, 70, 118 }, // Lv 70: ワールド・ダウン
  { 74, 80, 119 }, // Lv 80: エナジー・バーン
  // (はぐれノイズ who=75)
  { 75, 1, 120 },   // Lv 1: ノイズヒット
  { 75, 10, 121 },  // Lv 10: ソニックダガー
  { 75, 20, 122 },  // Lv 20: かく乱
  { 75, 30, 123 },  // Lv 30: ファントムラッシュ

  // (はぐれキング who=76)
  { 76, 40, 124 },   // Lv 1: キングスピア
  { 76, 50, 125 },  // Lv 15: ノイズストーム
  { 76, 60, 126 },  // Lv 30: ロイヤルチャージ
  { 76, 70, 127 }  // Lv 40: ラッキーセブン
};

const int Status::skillLearningRuleCount = sizeof(Status::skillLearningTable) / sizeof(Status::SkillLearningRule);

class Automaton {
public:
  Status* status;

  /**
   * @brief 仲間（オートマタ）を生成
   * @param level 初期レベル
   * @param w 仲間の種類ID
   */
  Automaton(int level, int w) {
    status = new Status(level, w);
  }

  const char* getname() {
    return status->getname();
  }

  int getWho() {
    return status->getWho();
  }

  ~Automaton() {
    delete status;
    status = nullptr;
  }
};

class Party {
public:
  // 仲間のポインタを保持するリスト
  std::vector<Automaton*> members;
  std::vector<Automaton*> storage;
  static const int MAX_MEMBERS = 3; // (例: 主人公を除き3体まで)

  Party() {
    members.reserve(MAX_MEMBERS); // メモリ予約
    storage.reserve(20);
  }

  // ★ 仲間を追加する関数
  int addMember(Automaton* newMember) {
    if (members.size() < MAX_MEMBERS) {
      members.push_back(newMember);
      return 1; // パーティー加入
    } else {
      storage.push_back(newMember);
      return 2; // 預かり所送還
    }
  }

  bool swapMember(int memberIdx, int storageIdx) {
    if (memberIdx < 0 || memberIdx >= members.size()) return false;
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    // ポインタを入れ替え
    Automaton* temp = members[memberIdx];
    members[memberIdx] = storage[storageIdx];
    storage[storageIdx] = temp;
    return true;
  }

  // ★★★ 預ける (Deposit) ★★★
  // パーティーから預かり所へ移動 (パーティーは減る)
  bool depositMember(int memberIdx) {
    if (memberIdx < 0 || memberIdx >= members.size()) return false;
    // 最後の1人は預けられない制約を入れても良い
    // if (members.size() <= 1) return false; 

    storage.push_back(members[memberIdx]);
    members.erase(members.begin() + memberIdx);
    return true;
  }

  // ★★★ 引き出す (Withdraw) ★★★
  // 預かり所からパーティーへ移動 (パーティーに空きが必要)
  bool withdrawMember(int storageIdx) {
    if (members.size() >= MAX_MEMBERS) return false; // 満員
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    members.push_back(storage[storageIdx]);
    storage.erase(storage.begin() + storageIdx);
    return true;
  }
  ~Party() {
    for (auto* m : members) delete m;
    members.clear();
    for (auto* s : storage) delete s;
    storage.clear();
  }
  // (将来的に、仲間を外す関数などもここに追加)
};

class Enemy{
public:
  Status* status;
  const char* name;
  int who;
  int dropItemId;
  int dropRatePercent;
  int xpYield;

  Enemy(int w, int floor){
    who = w;
    int level = floor + random(0, 3);
    if (level < 1) level = 1;
    
    // who ID に応じて敵のデータを変える
    switch (who) {
      // Enemy クラスのコンストラクタ内 switch (who)

      // --- 1. 基本5体 (Lv 3-5) ---
      case 0: // 攻撃型
        name = "ストライカー";
        status = new Status(level, who);
        dropItemId = 1; // 鉄くず (進化素材)
        dropRatePercent = 20; 
        xpYield = 5 + level;
        break;
      case 1: // 防御型
        name = "ガーディアン";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 20;
        xpYield = 5 + level;
        break;
      case 2: // 素早さ型
        name = "クリッパー";
        status = new Status(level, who);
        dropItemId = 1;
        dropRatePercent = 20;
        xpYield = 5 + level;
        break;
      case 3: // 回復型
        name = "メンダー";
        status = new Status(level, who);
        dropItemId = 100; // リペアキット
        dropRatePercent = 30;
        xpYield = 5 + level;
        break;
      case 4: // からめ手型
        name = "ジャマー";
        status = new Status(level, who);
        dropItemId = 2; // 銅の歯車 (換金)
        dropRatePercent = 30;
        xpYield = 5 + level;
        break;

      // --- 2. 1段階目進化 (Lv 10-12) ---
      case 5: // (ストライカー系)
        name = "ギア・ソルジャー";
        status = new Status(level, who);
        dropItemId = 1; // 鉄くず
        dropRatePercent = 30;
        xpYield = 20 + level * 2;
        break;
      case 6: // (ガーディアン系)
        name = "フォートレス";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 30;
        xpYield = 22 + level * 2;
        break;
      case 7: // (クリッパー系)
        name = "ブレード・ダンサー";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 30;
        xpYield = 18 + level * 2;
        break;
      case 8: // (メンダー系)
        name = "ハーモニスト";
        status = new Status(level, who);
        dropItemId = 103; // エネルギー缶
        dropRatePercent = 25;
        xpYield = 20 + level * 2;
        break;
      case 9: // (ジャマー系)
        name = "ディスアレンジャー";
        status = new Status(level, who);
        dropItemId = 2; // 銅の歯車
        dropRatePercent = 40;
        xpYield = 19 + level * 2;
        break;

      // --- 3. 2段階目進化 (Lv 20-22) ---
      case 10: // (攻撃型)
        name = "ギア・ナイト";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 50;
        break;
      case 11: // (防御型)
        name = "キャッスル・ウォール";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 55;
        break;
      case 12: // (素早さ型)
        name = "サイクロン・ダンサー";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 48;
        break;
      case 13: // (回復/旋律型)
        name = "コンダクター";
        status = new Status(level, who);
        dropItemId = 101; // リペアキット中
        dropRatePercent = 30;
        xpYield = 52;
        break;
      case 14: // (妨害/幸運型)
        name = "カオス・ジャマー";
        status = new Status(level, who);
        dropItemId = 104; // 万能オイル
        dropRatePercent = 30;
        xpYield = 49;
        break;

      // --- 4. 3段階目進化 (Lv 30-32) ---
      // ※ ここから「進化の歯車」を落とすように設定
      
      case 15: // ヴァンガード (攻撃の歯車)
        name = "ヴァンガード"; status = new Status(level, who); dropItemId = 10; dropRatePercent = 20; xpYield = 150; break;
      case 16: // スワッシュバックラー (赤の歯車)
        name = "スワッシュバックラー"; status = new Status(level, who); dropItemId = 11; dropRatePercent = 20; xpYield = 150; break;
      case 17: // バスティオン (防御の歯車)
        name = "バスティオン"; status = new Status(level, who); dropItemId = 13; dropRatePercent = 20; xpYield = 160; break;
      case 18: // イージス (青の歯車)
        name = "イージス"; dropItemId = 14; status = new Status(level, who); dropRatePercent = 20; xpYield = 170; break;
      case 19: // テンペスト (速度の歯車)
        name = "テンペスト"; dropItemId = 16; status = new Status(level, who); dropRatePercent = 20; xpYield = 145; break;
      case 20: // マエストロ (回復の歯車)
        name = "マエストロ"; dropItemId = 17; status = new Status(level, who); dropRatePercent = 20; xpYield = 155; break;
      case 21: // ディスコード (感応の歯車)
        name = "ディスコード"; dropItemId = 18; status = new Status(level, who); dropRatePercent = 20; xpYield = 150; break;

      // --- 5. 4段階目（最終）進化 (Lv 40-42) ---
      // ※ さらに上位の歯車や、レアアイテムを落とす
      
      case 22: // オーバーロード (赤の歯車)
        name = "オーバーロード"; dropItemId = 11; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 23: // デストロイヤー (紅の歯車)
        name = "デストロイヤー"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 24: // ゼロ・ブレード (紅の歯車)
        name = "ゼロ・ブレード"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 25: // ヴォイド・ストーカー (紅の歯車)
        name = "ヴォイド・ストーカー"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
        
      case 26: // アダマンタイト (青の歯車)
        name = "アダマンタイト"; dropItemId = 14; status = new Status(level, who); dropRatePercent = 15; xpYield = 420; break;
      case 27: // インヴィンシブル (蒼の歯車)
        name = "インヴィンシブル"; dropItemId = 15; status = new Status(level, who);dropRatePercent = 15; xpYield = 450; break;
      case 28: // ディヴァイン・シールド (蒼の歯車)
        name = "ディヴァイン・シールド"; dropItemId = 15; status = new Status(level, who); dropRatePercent = 15; xpYield = 430; break;
      case 29: // リジェネレーター (蒼の歯車)
        name = "リジェネレーター"; dropItemId = 15; status = new Status(level, who); dropRatePercent = 15; xpYield = 430; break;

      case 30: // シルフィード (速度の歯車)
        name = "シルフィード"; dropItemId = 16; status = new Status(level, who); dropRatePercent = 20; xpYield = 390; break;
      case 31: // アクセラレーター (銀の歯車)
        name = "アクセラレーター"; dropItemId = 3; status = new Status(level, who); dropRatePercent = 20; xpYield = 390; break;
        
      case 32: // アーク・ハーモニスト (回復の歯車)
        name = "アーク・ハーモニスト"; dropItemId = 17; status = new Status(level, who);dropRatePercent = 20; xpYield = 410; break;
      case 33: // シンフォニア (リペアキット大)
        name = "シンフォニア"; dropItemId = 102; status = new Status(level, who); dropRatePercent = 20; xpYield = 410; break;
        
      case 34: // パンデモニウム (感応の歯車)
        name = "パンデモニウム"; dropItemId =18; status = new Status(level, who);dropRatePercent = 30; xpYield = 400; break;
      case 35: // ジョーカー (謎のパーツ)
        name = "ジョーカー"; dropItemId = 5; status = new Status(level, who); dropRatePercent = 30; xpYield = 400; break;

      // --- 6. 特殊な敵 ---
      case 36: // はぐれノイズ (XP特化)
        name = "はぐれノイズ";
        status = new Status(5, who);
        dropItemId = 4; // 金の歯車
        dropRatePercent = 100; // 確定ドロップ
        xpYield = 10000; 
        break;

      case 37: // ノイズ・キング (XP特化)
        name = "ノイズ・キング";
        status = new Status(8, who);
        dropItemId = 504; // ラッキーコイン (アクセ)
        dropRatePercent = 10; // 激レア
        xpYield = 50000;
        break;

      case 80: 
        name = "スプラウト・ギア"; // 名前
        status = new Status(1, 80);
        status->setBossStats(500, 100, 40, 20, 15, 10, 10); // HP, TP, ATK, DEF, SPD, SEN, LCK
        status->forceLearnSkill(21);
        status->forceLearnSkill(106);
        status->forceLearnSkill(47);
        dropItemId = 500; // パワーリング
        dropRatePercent = 100; 
        xpYield = 500;
        break;

      // 20階ボス
      case 81: 
        name = "ソレイユ";
        status = new Status(1, 81);
        status->setBossStats(1200, 200, 70, 60, 10, 20, 20);
        status->forceLearnSkill(52);
        status->forceLearnSkill(34);
        status->forceLearnSkill(87);
        dropItemId = 501; // ガードリング
        dropRatePercent = 100;
        xpYield = 1500;
        break;

      // 30階ボス
      case 82: 
        name = "ハーベスト・スティンガー";
        status = new Status(1, 82);
        status->setBossStats(3000, 500, 120, 80, 60, 50, 40);
        // 状態異常3つ
        status->forceLearnSkill(105); // ポイズン (毒)
        status->forceLearnSkill(45);  // ヴォイド・ショック (麻痺)
        status->forceLearnSkill(113); // カオスフィールド (混乱)
        // 攻撃スキル
        status->forceLearnSkill(42);  // アサシネイト (確率で即死級ダメージ)
        dropItemId = 599; // トリニティ・コア
        dropRatePercent = 100; // 50%
        xpYield = 4000;
        break;

      // 40階ボス
      case 83: 
        name = "雪影";
        status = new Status(1, 83);
        status->setBossStats(8000, 999, 200, 150, 80, 100, 50);
        status->forceLearnSkill(73);  // クイックステップ (素早さUP)
        status->forceLearnSkill(39);  // ミラーズエッジ (回避率2倍)
        status->forceLearnSkill(109); // サイレンス (沈黙与) -> 詠唱を封じる
        status->forceLearnSkill(25);  // ソードダンス (攻撃・素早さUP)
        status->forceLearnSkill(123); // ファントムラッシュ (ランダム2~5回攻撃)
        status->forceLearnSkill(80);  // インビジブルエッジ (回避無視の必中攻撃)
        dropItemId = 399; // 創世のタクト
        dropRatePercent = 100;
        xpYield = 10000;
        break;

      // 50階 ラスボス
      case 84: 
        name = "デウス・エクス・マキナ";
        status = new Status(1, 84);
        status->setBossStats(20000, 9999, 300, 200, 120, 150, 100);
        status->forceLearnSkill(86);  // リミットブレイク (全能力最大強化)
        status->forceLearnSkill(118); // ワールド・ダウン (敵全体全能力DOWN)
        status->forceLearnSkill(114); // ディザスター (全状態異常付与)
        status->forceLearnSkill(57);  // 自己修復 (HPリジェネ)
        status->forceLearnSkill(64);  // 全体バリア (ダメージ1回無効)
        status->forceLearnSkill(65);  // 旋律反射 (魔法反射)
        status->forceLearnSkill(53);  // カウンター (物理反撃)
        status->forceLearnSkill(111); // TPドレイン (TP吸収)
        status->forceLearnSkill(127); // ラッキーセブン (ランダム1~8回攻撃)
        status->forceLearnSkill(126); // ロイヤルチャージ (溜めて次のターン超火力)
        dropItemId = 0; 
        dropRatePercent = 0;
        xpYield = 0;
        break;

      default: // 未定義ID
        who = 0; 
        name = "ストライカー";
        status = new Status(3, who);
        dropItemId = 1;
        dropRatePercent = 20;
        xpYield = 5;
        break;
    }
  }

  ~Enemy(){
    delete status;
    status = nullptr;
  }
};

class Caractor {
public:
  enum Direction { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };
  Direction direction = DIR_DOWN;
  int X, oldX, Y, oldY;
  bool moved = false;
  int MOVE_SPEED = 8;
  Status* status;
  // ★★★ 修正後のコンストラクタ ★★★
  // Mapオブジェクトを受け取り、安全なスタート地点を探す
  Caractor(Map& map, int level, int who) {
    status = new Status(level, who);
    bool positionFound = false;
    // 最初の草タイル(TILE_GRASS)を探す
    for (int y = 1; y < map.getMAP_HEIGHT() - 1; y++) {
      for (int x = 1; x < map.getMAP_WIDTH() - 1; x++) {
        if (map.mapData[y * map.getMAP_WIDTH() + x] == Map::TILE_GRASS) {
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
  
  void reset(Map& map) {
    // 以前の検索ループ処理は全て削除してください

    // Mapクラスの方で既にランダムに決められている座標(playerStartPos)をそのまま使う
    X = map.playerStartPos.x * Map::TILE_SIZE;
    Y = map.playerStartPos.y * Map::TILE_SIZE;

    oldX = X;
    oldY = Y;
  }
  ~Caractor() {
    if (status != nullptr) {
      delete status;
      status = nullptr;
    }
  }
};

// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------

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
  
  // 強制再描画フラグ
  int startTileX = -1; 
  // (以下の変数は関数内で計算するのでメンバ変数として保持する必要はないが、互換性のため残すなら残す)
  int startTileY = -1;
  int visTileW = 0; 
  int visTileH = 0;

  int currentFloorNumber = 1;
  void setFloor(int floor) {
    currentFloorNumber = floor;
  }
  // コンストラクタで lineSprite を初期化
  Graphic() : lineSprite(&lcd) {}

  void init(){
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

  // tileType からビットマップポインタを返す小ヘルパ
  inline const uint16_t* tileBitmapFor(int tileType){
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

  const uint16_t* getHeroImage(int dir) {
    switch(dir) {
      case Caractor::DIR_UP:    return (const uint16_t*)hero_up;
      case Caractor::DIR_LEFT:  return (const uint16_t*)hero_left;
      case Caractor::DIR_RIGHT: return (const uint16_t*)hero_right;
      case Caractor::DIR_DOWN:  
      default:                  return (const uint16_t*)hero_down;
    }
  }

  // ★ ラインバッファを使って背景全体を描画 (hero を引数に追加)
  void drawFullBackground(Map &map, const Caractor &hero, int cameraX_in, int cameraY_in){
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

            int tileType = map.mapData[ty * map.getMAP_WIDTH() + tx];
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
          //lineSprite.pushImage(heroScreenX, heroLocalY, Map::TILE_SIZE, Map::TILE_SIZE, (uint16_t*)hero_dots, H_TRANSPARENT);
          lineSprite.pushImage(heroScreenX, heroLocalY, Map::TILE_SIZE, Map::TILE_SIZE, (uint16_t*)heroBmp, H_TRANSPARENT);
        }

        // 5. 画面に転送
        lineSprite.pushSprite(0, drawY);
    }

    // 前回の位置を更新
    int hx = hero.X - cameraX;
    int hy = hero.Y - cameraY;
    prevHeroScreenX = hx;
    prevHeroScreenY = hy;
    
    // 再描画完了フラグ
    startTileX = 0; // -1 以外なら何でもOK
  }

  // 指定した矩形範囲(32x32)の背景をLCDに直接復元するヘルパー
  // (restorePrevHeroArea の代わり)
  void restoreRectDirect(Map &map, int sx, int sy) {
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
            int tileType = map.mapData[ty * map.getMAP_WIDTH() + tx];

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

  // smartRender (部分更新)
  // ヒーローが動いた時だけ、ヒーロー周辺だけを描き直す
  void smartRender(Map &map, const Caractor &hero, int desiredCameraX, int desiredCameraY){
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

    // ヒーローが動いていなければ何もしない
    if (prevHeroScreenX == heroScreenX && prevHeroScreenY == heroScreenY) return;

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
  }

  // 外部から強制再描画を要求するときに呼ぶ
  void invalidateAndRedraw(Map &map, const Caractor &hero, int desiredCameraX, int desiredCameraY){
    startTileX = -1; // フラグを立てる
    drawFullBackground(map, hero, desiredCameraX, desiredCameraY); // ★ hero を渡す
  }
};

class Controller {
public:
  const int BTN_UP=0, BTN_DOWN=3, BTN_LEFT=1, BTN_RIGHT=2;
  const int BTN_X=20, BTN_A=26, BTN_Y=27, BTN_B=21;
  uint32_t lastMoveTime = 0;
  const uint32_t moveIntervalMs = 80;

  // --- デバウンス用 ---
  const int btnPins[8] = {BTN_X, BTN_A, BTN_Y, BTN_B, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
  uint32_t lastPressTime[8];
  bool prevState[8];
  const uint32_t debounceMs = 180;

  Controller() {
    for (int i = 0; i < 8; ++i) {
      pinMode(btnPins[i], INPUT_PULLUP);
      lastPressTime[i] = 0;
      // ← ここを修正：prevState は「押されているか（LOW）」の boolean として保存する
      prevState[i] = (digitalRead(btnPins[i]) == LOW);
    }
  }

  // --- 通常押下（移動などに使う） ---
  bool pressed(int pin) {
    return digitalRead(pin) == LOW;
  }

  bool pressedDebounced(int pin) {
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
  
  // --- キャラ移動 ---
  void update_cara_point(Caractor &c) {
    uint32_t now = millis();
    if (now - lastMoveTime < moveIntervalMs) return; // まだ移動周期に達していない
    // 1ステップだけ進める（長押しで連続移動だが、moveIntervalMsで抑止）
    if (pressed(BTN_UP))    c.Y -= c.MOVE_SPEED;
    if (pressed(BTN_DOWN))  c.Y += c.MOVE_SPEED;
    if (pressed(BTN_LEFT))  c.X -= c.MOVE_SPEED;
    if (pressed(BTN_RIGHT)) c.X += c.MOVE_SPEED;
    lastMoveTime = now;
  }

  // --- 壁判定 ---
  void check_wall(Caractor &hero, Map & map){
    int hitbox_offset = 1;
    int centerX = hero.X + Map::TILE_SIZE/2;
    int centerY = hero.Y + Map::TILE_SIZE/2;
    int left = centerX - hitbox_offset, right = centerX + hitbox_offset;
    int top = centerY - hitbox_offset, bottom = centerY + hitbox_offset;
    if (map.mapData[top/Map::TILE_SIZE * map.getMAP_WIDTH() + left/Map::TILE_SIZE] == Map::TILE_WALL ||
        map.mapData[top/Map::TILE_SIZE * map.getMAP_WIDTH() + right/Map::TILE_SIZE] == Map::TILE_WALL ||
        map.mapData[bottom/Map::TILE_SIZE * map.getMAP_WIDTH() + left/Map::TILE_SIZE] == Map::TILE_WALL ||
        map.mapData[bottom/Map::TILE_SIZE * map.getMAP_WIDTH() + right/Map::TILE_SIZE] == Map::TILE_WALL) {
      hero.X = hero.oldX; hero.Y = hero.oldY; hero.moved = false;
    }
  }

  bool check_event(Caractor &hero, Map &map, GameState &state, EventType &currentEvent) {
    // 1. プレイヤーの現在タイル座標を取得
    int heroTileX = (hero.X + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
    int heroTileY = (hero.Y + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
    // 2. その場所のタイルタイプを取得
    uint8_t tileType = map.mapData[heroTileY * map.getMAP_WIDTH() + heroTileX];

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
};

class Vibration {
public:
  // ★ 振動モーターを繋ぐピン (回路に合わせて変更してください)
  static const int VIB_PIN = 7; 
  unsigned long vibrationEndTime = 0;
  bool _enabled = true;

  void init() {
    pinMode(VIB_PIN, OUTPUT);
    digitalWrite(VIB_PIN, LOW);
  }

  // msミリ秒だけ振動させる
  void trigger(int ms) {
    if (!_enabled) return;
    digitalWrite(VIB_PIN, HIGH);
    vibrationEndTime = millis() + ms;
  }

  void setEnabled(bool enabled) {
    _enabled = enabled;
  }
  // loop内で毎回呼ぶ
  void update() {
    if (vibrationEndTime > 0 && millis() > vibrationEndTime) {
      digitalWrite(VIB_PIN, LOW);
      vibrationEndTime = 0;
    }
  }
};

// -------------------------
// うまくいってるやつ
// -------------------------
/*class Menu {
public:
  // ★ メニューの状態を定義
  enum MenuState {
    STATE_MAIN,     // 道具、技、進化... を選ぶ
    STATE_EVOLVE    // ★ 「進化」画面
  };
  MenuState currentMenuState;

  // メインメニュー
  static const int MAIN_ITEM_COUNT = 4;
  // ★ "機構カスタマイズ" を "進化" に変更
  const char* mainItems[MAIN_ITEM_COUNT] = {"道具", "旋律", "進化", "設定"};
  int mainSelected = 0;

  // 進化画面
  int customizeSelected = 0; // 仲間リストのカーソル

  bool active = false;

  Menu() {
    currentMenuState = STATE_MAIN; // 初期状態
  }

  void toggle() { 
    active = !active; 
    currentMenuState = STATE_MAIN; // メニューを開くときは必ずメインから
    mainSelected = 0;
    customizeSelected = 0;
  }

  void update(Controller &ctrl, Inventory& inventory, Caractor& hero, Party& party) {
    if (!active) return; // メニューが非表示なら何もしない

    // --- Bボタン（キャンセル）処理 ---
    if (ctrl.pressedDebounced(ctrl.BTN_B)) {
      if (currentMenuState == STATE_MAIN) {
        // メインメニューでBを押したらメニューを閉じる
        active = false;
      } else if (currentMenuState == STATE_EVOLVE) {
        // 進化画面でBを押したらメインに戻る
        currentMenuState = STATE_MAIN;
      }
      return; // Bボタンが押されたフレームは他の操作を無視
    }

    // --- 状態別 Aボタン（決定）とカーソル移動 ---
    if (currentMenuState == STATE_MAIN) {
      // --- メインメニューの操作 ---
      
      // ★★★ バグ修正 ★★★
      // ゼロ除算 (% mainSelected) を修正
      if (ctrl.pressedDebounced(ctrl.BTN_UP))   mainSelected = (mainSelected - 1 + MAIN_ITEM_COUNT) % MAIN_ITEM_COUNT;
      if (ctrl.pressedDebounced(ctrl.BTN_DOWN)) mainSelected = (mainSelected + 1) % MAIN_ITEM_COUNT;
      // ★★★ 修正ここまで ★★★

      if (ctrl.pressedDebounced(ctrl.BTN_A)) {
        if (mainSelected == 2) { // 「進化」が選ばれた
          currentMenuState = STATE_EVOLVE;
          customizeSelected = 0; // カーソルをリセット
        }
        // (他の「道具(0)」や「旋律(1)」も将来ここで分岐)
      }

    } else if (currentMenuState == STATE_EVOLVE) {
      // --- 進化画面（仲間選択）の操作 ---
      int memberCount = party.members.size();
      if (memberCount > 0) { // 仲間が1人以上いる
        
        // 仲間リストのカーソル移動
        if (ctrl.pressedDebounced(ctrl.BTN_UP))   customizeSelected = (customizeSelected - 1 + memberCount) % memberCount;
        if (ctrl.pressedDebounced(ctrl.BTN_DOWN)) customizeSelected = (customizeSelected + 1) % memberCount;

        // Aボタン（決定）
        if (ctrl.pressedDebounced(ctrl.BTN_A)) {
          // ★ 選択した仲間を取得
          Automaton* selectedAlly = party.members[customizeSelected];
          
          // (今は仮に、アイテムID=1を持っているかチェック)
          bool hasEvoItem = false;
          int itemIndex = -1;
          for(int i=0; i < inventory.items.size(); i++) {
            if (inventory.items[i].id == 1) { // ID=1 (進化の歯車)
              hasEvoItem = true;
              itemIndex = i;
              break;
            }
          }

          // ★ もしアイテムを持っていて、進化可能なら
          if (hasEvoItem) {
            // (例) 仲間(who=39) を (who=40) に進化させる
            if (selectedAlly->who == 39) { 
              selectedAlly->status->evolve(40); // (Statusクラスのwho=40の定義が必要)
              // アイテムを消費
              inventory.items.erase(inventory.items.begin() + itemIndex);
            }
            // (ここに他の進化ルールを追加...)
          }
        } 
      }
    } 
  }

  void draw(LGFX_Sprite &sp, Inventory& inventory, Caractor& hero, Party& party) {
    if (!active) return;
    sp.fillScreen(TFT_BLACK);

    if (currentMenuState == STATE_MAIN) {
      // --- メインメニューの描画 ---
      int x = 60, y = 40, w = 200, h = 140;
      sp.fillRect(x, y, w, h, TFT_DARKGREY);
      sp.drawRect(x, y, w, h, TFT_WHITE);
      sp.setTextSize(2);
      for (int i = 0; i < MAIN_ITEM_COUNT; i++) {
        uint16_t color = (i == mainSelected) ? TFT_YELLOW : TFT_WHITE;
        sp.setTextColor(color);
        sp.setCursor(x + 20, y + 20 + i * 28);
        sp.print(mainItems[i]);
      }

    } else if (currentMenuState == STATE_EVOLVE) {
      // --- 進化画面の描画 ---
      int x = 10, y = 40, w = 300, h = 190;
      sp.fillRect(x, y, w, h, TFT_DARKGREY);
      sp.drawRect(x, y, w, h, TFT_WHITE);
      sp.setTextSize(2);
      
      sp.setTextColor(TFT_WHITE);
      sp.setCursor(x + 20, y + 15);
      sp.print("進化 (誰を？)");
      sp.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

      // --- 仲間リストの描画 ---
      int memberCount = party.members.size();
      if (memberCount == 0) {
        sp.setCursor(x + 20, y + 55);
        sp.print("仲間がいません");
      } else {
        for (int i = 0; i < memberCount; i++) {
          Automaton* ally = party.members[i];
          uint16_t color = (i == customizeSelected) ? TFT_YELLOW : TFT_WHITE;
          sp.setTextColor(color);
          sp.setCursor(x + 20, y + 55 + i * 28);
          sp.printf("%s Lv%d HP%d/%d", 
            ally->name.c_str(), 
            ally->status->getLevel(), 
            ally->status->getHp(), 
            ally->status->getMaxHp()
          );
        }
      }
      
      // --- 所持アイテム表示 (例) ---
      sp.drawLine(x, y + 140, x + w, y + 140, TFT_WHITE);
      sp.setTextColor(TFT_WHITE);
      sp.setCursor(x + 20, y + 155);
      sp.print("必要なアイテム: 進化の歯車");
    }
  }
};*/

// -------------------------
// AUDIO / SD (core1 に限定)
// -------------------------
bool audio_timer_callback(struct repeating_timer *t);
class Sd;
class MUSIC {
public:
  static const int BUF_SIZE = 4096;
  static const int TRACK_COUNT = 100;  // 例: 0=nocturne, 1=gekkou

  FsFile musicFiles[TRACK_COUNT];
  int currentTrack = 0;

  volatile uint8_t buffer0[BUF_SIZE];
  volatile uint8_t buffer1[BUF_SIZE];
  volatile uint8_t* playBuf = buffer0;
  volatile uint8_t* fillBuf = buffer1;

  volatile size_t playBytes = 0;
  volatile size_t fillBytes = 0;
  volatile size_t sampleIndex = 0;

  volatile bool needs_refill = true;
  volatile bool paused = false;
  // ★★★ 修正: SE用バッファを全削除し、シンセ用変数を追加 ★★★
  
  // 波形タイプ
  enum WaveType { OFF, SQUARE, NOISE };
  
  volatile WaveType seType = OFF; // 現在鳴っている音の種類
  volatile float sePhase = 0;     // 波形の進行度
  volatile float seFreq = 0;      // 周波数 (高さ)
  volatile float seFreqSlide = 0; // 周波数の変化量 (ピュイッ、のような効果)
  volatile float seVol = 0;       // 音量
  volatile float seVolDecay = 0;  // 音量の減衰量 (フェードアウト)
  volatile int targetSeVol = 10;
  // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
  float bgmVolume = 1.0;
  int masterSeVol = 10;
  void setSeVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 10) vol = 10;
    masterSeVol = vol;
  }
  MUSIC() {
    // DAC ピン初期化（core1 でもやるがここで safe）
    pinMode(DAC_CS_PIN, OUTPUT);
    pinMode(DAC_SCK_PIN, OUTPUT);
    pinMode(DAC_MOSI_PIN, OUTPUT);
    digitalWrite(DAC_CS_PIN, HIGH);
    // SE変数の初期化
    // ★★★ 修正: シンセサイザー変数の初期化 ★★★
    // (古いループ処理は削除して、以下に置き換える)
    seType = OFF;
    sePhase = 0;
    seFreq = 0;
    seFreqSlide = 0;
    seVol = 0;
    seVolDecay = 0;
    // ★★★★★★★★★★★★★★★★★★★★★★★★★
  }
  // MUSIC クラス内
  
  void playSE(int id) {
      sePhase = 0;
      
      // ★修正: 設定値(0-10)を反映させる
      // masterSeVol が 10 なら 5000.0、5 なら 2500.0、0 なら 0.0
      float baseVol = 5000.0f * ((float)masterSeVol / 10.0f);
      seVol = baseVol;
      //seVol = 5000.0; // 基本音量

      switch (id) {
          case 0: // 決定 (ピロリン)
              seType = SQUARE; 
              seFreq = 880; 
              seFreqSlide = 0.5; // ほんの少し上がる
              seVolDecay = 4.0;  // 0.1秒くらいで消える
              break;

          case 1: // キャンセル (ブッ)
              seType = SQUARE; 
              seFreq = 150; 
              seFreqSlide = -0.1; 
              seVolDecay = 8.0;   // 早めに消える
              break;

          case 2: // カーソル (ッ)
              seType = SQUARE; 
              seFreq = 2000; 
              seFreqSlide = 0; 
              seVolDecay = 20.0; // 一瞬
              break;

          case 3: // 通常攻撃 (ザシュ)
              seType = NOISE; 
              seFreq = 0; 
              seFreqSlide = 0; 
              seVolDecay = 2.0; // 0.25秒くらい
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          case 4: // 会心 (ギャイン！)
              seType = NOISE; 
              seFreq = 0; 
              seFreqSlide = 0; 
              seVolDecay = 0.5; // 1秒くらい響
              seVol = 4000 * ((float)masterSeVol / 10.0f);
              break;

          case 5: // 物理技 (ドカッ)
              seType = SQUARE; 
              seFreq = 100; 
              seFreqSlide = -0.2; 
              seVolDecay = 1.0;
              break;

          case 6: // 物理全体 (ザーッ)
              seType = NOISE; 
              seFreq = 0; 
              seFreqSlide = 0; 
              seVolDecay = 1.0; // 長め
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          case 7: // 魔法/回復 (ヒュン)
              seType = SQUARE; 
              seFreq = 600; 
              seFreqSlide = 2.0; // キュイーンと上がる
              seVolDecay = 2.5;
              break;

          case 8: // 魔法全体 (ジャーン)
              seType = NOISE; 
              seFreq = 0; 
              seFreqSlide = 0; 
              seVolDecay = 1.5;
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          default:
              seType = OFF;
              break;
      }
  }
  // 最速 bit-bang 用（ISR から呼ぶ。gpio_put を使う）
  inline void softSPIWrite_fast(uint16_t value) {
    uint32_t mask = (1u << DAC_MOSI_PIN);
    uint32_t clk  = (1u << DAC_SCK_PIN);
    for (int i = 15; i >= 0; --i) {
      if (value & (1 << i))
        sio_hw->gpio_set = mask;
      else
        sio_hw->gpio_clr = mask;
      sio_hw->gpio_set = clk;
      sio_hw->gpio_clr = clk;
    }
  }
  
  inline void setDAC_ISR(uint16_t value) {
    uint16_t dac_data = 0x3000 | (value & 0x0FFF);
    uint32_t csMask = (1u << DAC_CS_PIN);
    sio_hw->gpio_clr = csMask;
    softSPIWrite_fast(dac_data);
    sio_hw->gpio_set = csMask;
  }

  void switchTrack(int newId, class Sd &sd);
};

class Sd {
public:
  volatile bool pendingSwitch = false;
  int pendingTrack = -1;
  size_t refillOffset = 0;
  SdFat sdfat;
  absolute_time_t refill_block_until = 0;
  Sd() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
  }
  void resetRefillOffset() { refillOffset = 0; }
  void init(MUSIC &music) {
    if (!sdfat.begin(SD_CS_PIN, SD_SCK_MHZ(20))) {
      //Serial.println("SD init failed!");
      while (1);
    }

    const char* paths[MUSIC::TRACK_COUNT];
    for(int i=0; i<MUSIC::TRACK_COUNT; i++) paths[i] = "";
    paths[0] = "/bgm_0.raw"; // 集落
    paths[1] = "/bgm_1.raw"; // ダンジョン
    paths[2] = "/bgm_2.raw"; // 通常戦闘
    paths[3] = "/bgm_3.raw"; // ボス戦
    paths[4] = "/bgm_4.raw"; // ラスボス
    paths[9] = "/bgm_9.raw"; // タイトル
    paths[99] = "/bgm_99.raw";
    for (int i = 0; i < MUSIC::TRACK_COUNT; ++i) {
      music.musicFiles[i] = sdfat.open(paths[i], FILE_READ);
      if (!music.musicFiles[i]) {
        //Serial.printf("Failed to open %s\n", paths[i]);
        //while (1);
      }
    }
    music.currentTrack = 9;
    // 初期曲データ読み込み
    FsFile& f = music.musicFiles[music.currentTrack];
    f.seek(0);
    size_t n0 = f.read((void*)music.buffer0, MUSIC::BUF_SIZE);
    size_t n1 = f.read((void*)music.buffer1, MUSIC::BUF_SIZE);
    noInterrupts();
    music.playBuf = music.buffer0;
    music.fillBuf = music.buffer1;
    music.playBytes = n0;
    music.fillBytes = n1;
    music.sampleIndex = 0;
    music.needs_refill = false;
    interrupts();
  }

  // 切り替え予約（ID指定）
  void requestSwitch(int trackId) {
    if (pendingSwitch) return;
    noInterrupts();
    pendingTrack = trackId;
    pendingSwitch = true;
    interrupts();
  }

  // core1ループで実行
  void performPendingSwitch(MUSIC &music) {
    if (!pendingSwitch) return;

    // --- 競合防止ロック ---
    critical_section_enter_blocking(&sdLock);

    int newTrack;
    noInterrupts();
    newTrack = pendingTrack;
    pendingSwitch = false;
    interrupts();

    music.paused = true;

    // --- バッファ完全リセット ---
    memset((void*)music.buffer0, 0, MUSIC::BUF_SIZE);
    memset((void*)music.buffer1, 0, MUSIC::BUF_SIZE);
    music.sampleIndex = 0;
    music.playBytes = 0;
    music.fillBytes = 0;
    music.needs_refill = false;
    refillOffset = 0;

    // --- ファイルを最初から ---
    FsFile &file = music.musicFiles[newTrack];
    file.seek(0);

    // --- 新しいデータをロード ---
    size_t n0 = file.read((void*)music.buffer0, MUSIC::BUF_SIZE);
    size_t n1 = file.read((void*)music.buffer1, MUSIC::BUF_SIZE);

    // --- 原子的にスイッチ ---
    noInterrupts();
    music.currentTrack = newTrack;
    music.playBuf = music.buffer0;
    music.fillBuf = music.buffer1;
    music.playBytes = n0;
    music.fillBytes = n1;
    music.sampleIndex = 0;
    music.needs_refill = false;
    interrupts();

    // --- ロック解除 ---
    critical_section_exit(&sdLock);

    delay(30);  // ISRが落ち着く時間
    music.paused = false;
  }

  void refile(MUSIC &music) {
    // only refill when requested
    if (!music.needs_refill) return;
    // --- safety guard ---
    if (music.paused || refillOffset == 0 && music.sampleIndex == 0 && music.playBytes == MUSIC::BUF_SIZE)
      return;

    // If we just switched tracks and refillOffset==0 and playBytes>0 and sampleIndex==0,
    // then we've already prefilled buffers in performPendingSwitch — don't refile immediately.
    if (refillOffset == 0 && music.sampleIndex == 0 && music.playBytes > 0) {
      // keep needs_refill false until playback advances a bit
      // but being defensive: only delay for one buffer consumption threshold
      return;
    }

    FsFile& file = music.musicFiles[music.currentTrack];
    if (!file.isOpen()) return;

    critical_section_enter_blocking(&sdLock);
    const size_t CHUNK = 2048;
    bool eofReached = false;

    if (refillOffset < MUSIC::BUF_SIZE) {
      size_t toRead = min(CHUNK, MUSIC::BUF_SIZE - refillOffset);
      size_t n = file.read((void*)(music.fillBuf + refillOffset), toRead);

      if (n == 0 && refillOffset == 0) {
        memset((void*)(music.fillBuf + refillOffset), 0, MUSIC::BUF_SIZE - refillOffset);
        eofReached = true;
      } else if (n == 0) {
        // loop file
        file.seek(0);
        refillOffset = 0;
        eofReached = true;
      } else {
        refillOffset += n;
      }
    }
    critical_section_exit(&sdLock);

    if (refillOffset >= MUSIC::BUF_SIZE || eofReached) {
      noInterrupts();
      music.fillBytes = refillOffset;
      music.needs_refill = false;
      interrupts();
      refillOffset = 0;
    }
  }

  bool loadToBuffer(String filename, uint16_t* buffer) {
    FsFile file = sdfat.open(filename.c_str(), O_READ);
    if (!file) return false; // 失敗

    // 指定された場所にデータを流し込む
    file.read((void*)buffer, 64 * 64 * 2);
    file.close();
    return true;
  }
};
// --- MUSIC::switchTrack 実装部（Sdの定義後） ---
void MUSIC::switchTrack(int newId, Sd &sd) {
  if (newId == currentTrack) return;

  // --- 安全な切替要求 ---
  paused = true;

  // SDロックを取得（refileと競合しないようにする）
  critical_section_enter_blocking(&sdLock);
  sampleIndex = 0;
  needs_refill = false;
  critical_section_exit(&sdLock);

  sd.requestSwitch(newId);
}

class Encounter {
public:
  int stepCounter = 0;
  const int encounterThreshold = 20; // 歩数間隔の平均
  bool checkEncounter() {
    stepCounter++;
    // 10歩〜30歩の間で確率的にエンカウント
    if (stepCounter > random(10, encounterThreshold)) {
      stepCounter = 0;
      return random(100) < 10; // 10% の確率で戦闘
    }
    return false;
  }
};

// ---------
// saveクラス
// ---------

class SaveManager {
public:
  const char* SAVE_FILENAME = "/save.dat";
  const char* SAVE_MAGIC = "PGSAVE2";

  // ★ セーブ実行
  bool saveGame(
      Sd& sd,
      MUSIC& music,
      Map& map,
      Caractor& hero,
      Party& party,
      Inventory& inventory,
      bool* bossFlags
  ) {
    music.paused = true;
    critical_section_enter_blocking(&sdLock);

    FsFile file =
        sd.sdfat.open(
            SAVE_FILENAME,
            O_WRITE | O_CREAT | O_TRUNC
        );

    if (!file) {
      critical_section_exit(&sdLock);
      music.paused = false;
      return false;
    }

    // -------------------------------------------------
    // セーブ形式
    // -------------------------------------------------
    file.println(SAVE_MAGIC);

    // -------------------------------------------------
    // 1. 現在のマップ状態
    // -------------------------------------------------
    file.println(map.currentFloor);
    file.println((int)map.currentType);

    file.println(map.MAP_WIDTH);
    file.println(map.MAP_HEIGHT);

    // マップ上の未回収オートマタの種類
    file.println(map.automatonWhoId);

    // 主人公の現在位置・向き
    file.println(hero.X);
    file.println(hero.Y);
    file.println((int)hero.direction);

    // マップそのもの
    file.println((int)map.mapData.size());

    for (uint8_t tile : map.mapData) {
      file.println((int)tile);
    }

    // -------------------------------------------------
    // 2. 主人公
    // -------------------------------------------------
    hero.status->saveTo(file);

    // -------------------------------------------------
    // 3. インベントリ
    // -------------------------------------------------
    inventory.saveTo(file);

    // -------------------------------------------------
    // 4. パーティー
    // -------------------------------------------------
    file.println(party.members.size());

    for (auto* member : party.members) {
      member->status->saveTo(file);
    }

    // -------------------------------------------------
    // 5. 預かり所
    // -------------------------------------------------
    file.println(party.storage.size());

    for (auto* member : party.storage) {
      member->status->saveTo(file);
    }

    // -------------------------------------------------
    // 6. ボス撃破状態
    // -------------------------------------------------
    for (int i = 0; i < 5; i++) {
      file.println(bossFlags[i]);
    }

    file.close();

    critical_section_exit(&sdLock);
    music.paused = false;

    return true;
  }

  // ★ ロード実行
  bool loadGame(
      Sd& sd,
      MUSIC& music,
      Map& map,
      Caractor& hero,
      Party& party,
      Inventory& inventory,
      bool* bossFlags
  ) {
    music.paused = true;
    critical_section_enter_blocking(&sdLock);

    FsFile file =
        sd.sdfat.open(
            SAVE_FILENAME,
            O_READ
        );

    if (!file) {
      critical_section_exit(&sdLock);
      music.paused = false;
      return false;
    }

    // ロード失敗時の共通終了処理
    auto failLoad = [&]() -> bool {
      file.close();
      critical_section_exit(&sdLock);
      music.paused = false;
      return false;
    };

    // -------------------------------------------------
    // セーブ形式判定
    // -------------------------------------------------
    String firstLine =
        file.readStringUntil('\n');

    bool isNewFormat =
        (firstLine == SAVE_MAGIC);

    Map::MapType loadedType =
        Map::TYPE_DUNGEON;

    int savedHeroX = 0;
    int savedHeroY = 0;
    int savedDirection =
        (int)Caractor::DIR_DOWN;

    if (isNewFormat) {
      // -------------------------------------------------
      // PGSAVE2
      // -------------------------------------------------
      map.currentFloor =
          file.readStringUntil('\n').toInt();

      int typeVal =
          file.readStringUntil('\n').toInt();

      if (typeVal < (int)Map::TYPE_MAZE ||
          typeVal > (int)Map::TYPE_BOSS_ROOM)
      {
        return failLoad();
      }

      loadedType =
          static_cast<Map::MapType>(
              typeVal
          );

      int savedWidth =
          file.readStringUntil('\n').toInt();

      int savedHeight =
          file.readStringUntil('\n').toInt();

      // 異常なセーブデータで巨大なvectorを確保しないための保護
      if (savedWidth <= 0 ||
          savedHeight <= 0 ||
          savedWidth > 64 ||
          savedHeight > 64)
      {
        return failLoad();
      }

      map.automatonWhoId =
          file.readStringUntil('\n').toInt();

      savedHeroX =
          file.readStringUntil('\n').toInt();

      savedHeroY =
          file.readStringUntil('\n').toInt();

      savedDirection =
          file.readStringUntil('\n').toInt();

      if (savedDirection < (int)Caractor::DIR_DOWN ||
          savedDirection > (int)Caractor::DIR_RIGHT)
      {
        return failLoad();
      }

      int mapDataCount =
          file.readStringUntil('\n').toInt();

      if (mapDataCount !=
          savedWidth * savedHeight)
      {
        return failLoad();
      }

      if (savedHeroX < 0 ||
          savedHeroY < 0 ||
          savedHeroX >= savedWidth * Map::TILE_SIZE ||
          savedHeroY >= savedHeight * Map::TILE_SIZE)
      {
        return failLoad();
      }

      map.currentType =
          loadedType;

      map.MAP_WIDTH =
          savedWidth;

      map.MAP_HEIGHT =
          savedHeight;

      map.isTown =
          (loadedType == Map::TYPE_TOWN);

      map.mapData.assign(
          mapDataCount,
          Map::TILE_WALL
      );

      for (int i = 0;
          i < mapDataCount;
          i++)
      {
        int tile =
            file.readStringUntil('\n').toInt();

        if (tile < 0 || tile > 255) {
          return failLoad();
        }

        map.mapData[i] =
            (uint8_t)tile;
      }
    }
    else {
      // -------------------------------------------------
      // 旧形式
      //
      // 旧セーブにはマップ本体がないため、
      // 従来どおり階層とMapTypeだけ読み、
      // 最後にマップを再生成する。
      // -------------------------------------------------
      map.currentFloor =
          firstLine.toInt();

      int typeVal =
          file.readStringUntil('\n').toInt();

      if (typeVal < (int)Map::TYPE_MAZE ||
          typeVal > (int)Map::TYPE_BOSS_ROOM)
      {
        return failLoad();
      }

      loadedType =
          static_cast<Map::MapType>(
              typeVal
          );
    }

    // -------------------------------------------------
    // 主人公
    // -------------------------------------------------
    hero.status->loadFrom(file);

    // -------------------------------------------------
    // インベントリ
    // -------------------------------------------------
    inventory.loadFrom(file);

    // -------------------------------------------------
    // パーティー
    // -------------------------------------------------
    for (auto* member : party.members) {
      delete member;
    }

    party.members.clear();

    int memberCount =
        file.readStringUntil('\n').toInt();

    for (int i = 0;
        i < memberCount;
        i++)
    {
      Automaton* ally =
          new Automaton(1, 0);

      ally->status->loadFrom(file);

      party.members.push_back(ally);
    }

    // -------------------------------------------------
    // 預かり所
    // -------------------------------------------------
    for (auto* member : party.storage) {
      delete member;
    }

    party.storage.clear();

    int storageCount =
        file.readStringUntil('\n').toInt();

    for (int i = 0;
        i < storageCount;
        i++)
    {
      Automaton* ally =
          new Automaton(1, 0);

      ally->status->loadFrom(file);

      party.storage.push_back(ally);
    }

    // -------------------------------------------------
    // ボス撃破状態
    // -------------------------------------------------
    for (int i = 0; i < 5; i++) {
      bossFlags[i] =
          (
            file.readStringUntil('\n').toInt()
            != 0
          );
    }

    // -------------------------------------------------
    // マップ・主人公位置の復元
    // -------------------------------------------------
    if (isNewFormat) {
      // 新形式は保存したマップをそのまま使用する。
      // regenerate() は絶対に呼ばない。

      hero.X =
          savedHeroX;

      hero.Y =
          savedHeroY;

      hero.oldX =
          savedHeroX;

      hero.oldY =
          savedHeroY;

      hero.direction =
          (Caractor::Direction)
              savedDirection;

      hero.moved = false;
    }
    else {
      // 旧形式はマップ本体を持っていないので
      // 従来どおり再生成する。

      Map::MapType finalType =
          loadedType;

      // ボス階ではbossFlagsを優先
      if (map.currentFloor >= 10 &&
          map.currentFloor <= 50 &&
          map.currentFloor % 10 == 0)
      {
        int bossIndex =
            (map.currentFloor / 10) - 1;

        if (bossIndex >= 0 &&
            bossIndex < 5)
        {
          if (bossFlags[bossIndex]) {
            finalType =
                Map::TYPE_TOWN;
          }
          else {
            finalType =
                Map::TYPE_BOSS_ROOM;
          }
        }
      }

      map.regenerate(
          20,
          20,
          finalType
      );

      map.isTown =
          (finalType == Map::TYPE_TOWN);

      hero.reset(map);
    }

    file.close();

    critical_section_exit(&sdLock);
    music.paused = false;

    return true;
  }
  
  // ファイルが存在するかチェック
  bool hasSaveFile(Sd& sd) {
     return sd.sdfat.exists(SAVE_FILENAME);
  }
};

// main.ino (1001行目あたり)
struct Config {
  int bgmVolume = 10; // 0〜10 (初期値5)
  int seVolume = 10;  // 0〜10 (初期値5)
  bool vibration = true; // ON/OFF
};

class Menu {
public:
  // ★ メニューの状態を定義
  enum MenuState {
    STATE_MAIN,     // 道具、技、進化... を選ぶ
    STATE_EVOLVE,    // ★ 「進化」画面
    STATE_SKILL,
    STATE_ITEM,
    STATE_ITEM_TARGET, // ★ 道具の対象選択
    STATE_STORAGE, // ★ 預かり所画面
    STATE_SAVE_RESULT,
    // --- 装備用 ---
    STATE_EQUIP_SLOT,   // どの部位？ (武器/防具/装飾)
    STATE_EQUIP_LIST,     // 何を装備する？
    // ★★★ 追加: ショップと宿屋 ★★★
    STATE_INN,          // 宿屋 (泊まりますか？)
    STATE_SHOP_CHOICE,  // 店 (買う/売る/出る)
    STATE_SHOP_BUY,     // 買う画面
    STATE_SHOP_SELL,     // 売る画面
    STATE_CONFIG
    // ★★★★★★★★★★★★★★★★★★★
  };
  MenuState currentMenuState;

  // メインメニュー
  static const int MAIN_ITEM_COUNT = 6;
  // ★ "機構カスタマイズ" を "進化" に変更
  const char* mainItems[MAIN_ITEM_COUNT] = {"道具", "旋律", "進化", "装備", "記録", "設定"};
  int mainSelected = 0;
  // 進化画面
  int listCursor = 0;
  bool active = false;
  bool saveSucceeded = false;
  std::vector<int> shopItemList;
  int selectedItemIndex = -1; // 選択中の道具のインデックス保存用
  int storageMode = 0; // 0:メニュー選択, 1:引き出す, 2:預ける

  int equipSlot = 0;        // 0:武器, 1:防具, 2:装飾
  Menu() {
    currentMenuState = STATE_MAIN; // 初期状態
  }

  void toggle() { 
    active = !active; 
    currentMenuState = STATE_MAIN; // メニューを開くときは必ずメインから
    mainSelected = 0;
    listCursor = 0;
  }

  void update(Controller &ctrl, Inventory& inventory, Caractor& hero, Party& party, Map& map, MUSIC& music, Sd& sd, SaveManager& saveMgr, Config& config, Vibration& vib, bool* bossFlags);
  
  void draw(Graphic* g, Inventory& inventory, Caractor& hero, Party& party, Config& config) {
    if (!active) return;

    // ★ スプライト生成をやめて g->lineSprite を使う

    // --- 分割描画ループ ---
    for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {
      g->lineSprite.fillScreen(TFT_BLACK);
      int yOffset = -drawY;
      // 各要素の描画 (Y座標から drawY を引く)
      // ※ 以下、sp. を g->lineSprite. に置換
      
      if (currentMenuState == STATE_MAIN) {
        int x = 60, y = 40 - drawY; 
        int w = 200, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        for (int i = 0; i < MAIN_ITEM_COUNT; i++) {
          uint16_t color = (i == mainSelected) ? TFT_YELLOW : TFT_WHITE;
          g->lineSprite.setTextColor(color);
          g->lineSprite.setCursor(x + 20, y + 20 + i * 28);
          g->lineSprite.print(mainItems[i]);
        }

      } else if (currentMenuState == STATE_EVOLVE) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("進化 (誰を？)");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          int memberCount = party.members.size();
          if (memberCount == 0) {
              g->lineSprite.setCursor(x + 20, y + 55);
              g->lineSprite.print("仲間がいません");
          } else {
              for (int i = 0; i < memberCount; i++) {
                  if (i >= 5) break; 
                  Automaton* ally = party.members[i];
                  uint16_t color = (i == listCursor) ? TFT_YELLOW : TFT_WHITE;
                  g->lineSprite.setTextColor(color);
                  g->lineSprite.setCursor(x + 20, y + 55 + i * 28);
                  g->lineSprite.printf("%s Lv%d HP%d/%d", ally->getname(), ally->status->getLevel(), ally->status->getHp(), ally->status->getMaxHp());
              }
          }

      } else if (currentMenuState == STATE_SKILL) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("旋律 (何を習得？)");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          int itemsPerPage = 3;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = startIdx + itemsPerPage;
          if (endIdx > learnableSkillCount) endIdx = learnableSkillCount;

          for (int i = startIdx; i < endIdx; i++) {
              Skill skill(learnableSkillIDs[i]);
              int scrollId = 200 + skill.id;
              bool hasScroll = (inventory.countItem(scrollId) > 0);
              bool alreadyLearned = false;
              for(const auto& s : hero.status->getLearnedSkills()){
                  if(s.id == skill.id) { alreadyLearned = true; break; }
              }
              uint16_t color = TFT_WHITE;
              if (i == listCursor) color = TFT_YELLOW;
              else if (alreadyLearned) color = TFT_GREEN;
              else if (!hasScroll) color = TFT_LIGHTGREY;

              g->lineSprite.setTextColor(color);
              g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
              String statusStr = "";
              if (alreadyLearned) statusStr = "(済)";
              g->lineSprite.printf("%s%s (TP:%d)", skill.name, statusStr.c_str(), skill.tpCost);
          }
          
          g->lineSprite.drawLine(x, y + 140, x + w, y + 140, TFT_WHITE);
          int targetSkillId = learnableSkillIDs[listCursor];
          int reqScrollId = 200 + targetSkillId;
          int scrollCount = inventory.countItem(reqScrollId);
          g->lineSprite.setCursor(x + 20, y + 155);
          g->lineSprite.setTextColor(TFT_WHITE);
          bool isLearned = false;
          for(const auto& s : hero.status->getLearnedSkills()){ if(s.id == targetSkillId) { isLearned = true; break; } }
          if (isLearned) { g->lineSprite.setTextColor(TFT_GREEN); g->lineSprite.print("習得済みです"); } 
          else if (scrollCount > 0) { g->lineSprite.setTextColor(TFT_CYAN); g->lineSprite.printf("習得可能！ (所持: %d)", scrollCount); } 
          else { g->lineSprite.setTextColor(TFT_RED); g->lineSprite.setTextSize(1); g->lineSprite.setCursor(x + 20, y + 160); g->lineSprite.printf("必要: %s", createItemById(reqScrollId).name); }

      } else if (currentMenuState == STATE_ITEM) {
        int x = 10, y = 40 - drawY; 
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("道具 (一覧)");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        int itemCount = inventory.items.size();
        if (itemCount == 0) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("道具を持っていません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = startIdx + itemsPerPage;
          if (endIdx > itemCount) endIdx = itemCount;

          for (int i = startIdx; i < endIdx; i++) {
            int id = inventory.items[i].item.id;
            bool isUsable = (id >= 100 && id < 200);
            uint16_t color = TFT_WHITE;
            if (i == listCursor) color = TFT_YELLOW;
            else if (!isUsable) color = TFT_LIGHTGREY;

            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            if (id >= 200 && id < 300) {
              // %sの巻物 と書くことで、画面上だけ結合される
              g->lineSprite.printf("%sの巻物 x%d", inventory.items[i].item.name, inventory.items[i].quantity);
            } else {
              // 通常アイテム
              g->lineSprite.printf("%s x%d", inventory.items[i].item.name, inventory.items[i].quantity);
            }
          }
        }

      } else if (currentMenuState == STATE_ITEM_TARGET) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("誰に使いますか？");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          uint16_t color = (listCursor == 0) ? TFT_YELLOW : TFT_WHITE;
          g->lineSprite.setTextColor(color);
          g->lineSprite.setCursor(x + 20, y + 55);
          g->lineSprite.printf("調律師 HP:%d/%d", hero.status->getHp(), hero.status->getMaxHp());

          for (int i = 0; i < party.members.size(); i++) {
            color = (listCursor == i + 1) ? TFT_YELLOW : TFT_WHITE;
            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x + 20, y + 55 + (i + 1) * 28);
            auto* s = party.members[i]->status;
            g->lineSprite.printf("%s HP:%d/%d", s->getname(), s->getHp(), s->getMaxHp());
          }

      } else if (currentMenuState == STATE_STORAGE) {
          g->lineSprite.fillScreen(TFT_BLACK); 
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setCursor(10, 10 - drawY);
          g->lineSprite.print("=== 預かり所 ===");

          if (storageMode == 0) {
              g->lineSprite.setCursor(40, 50 - drawY);
              g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE); g->lineSprite.print("仲間を引き出す");
              g->lineSprite.setCursor(40, 80 - drawY);
              g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE); g->lineSprite.print("仲間を預ける");
              g->lineSprite.setTextColor(TFT_WHITE);
              g->lineSprite.setCursor(40, 130 - drawY);
              g->lineSprite.printf("パーティー: %d/%d人", party.members.size(), Party::MAX_MEMBERS);
              g->lineSprite.setCursor(40, 150 - drawY);
              g->lineSprite.printf("預かり所: %d人", party.storage.size());
          }
          else if (storageMode == 1) {
              g->lineSprite.setCursor(10, 35 - drawY); g->lineSprite.print("誰を引き出しますか？");
              if (party.members.size() >= Party::MAX_MEMBERS) { g->lineSprite.setTextColor(TFT_RED); g->lineSprite.print(" (満員)"); }
              int count = party.storage.size();
              if (count == 0) { g->lineSprite.setCursor(30, 60 - drawY); g->lineSprite.setTextColor(TFT_LIGHTGREY); g->lineSprite.print("誰もいません"); }
              
              int itemsPerPage = 6; 
              int startIdx = 0;
              if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
              int endIdx = startIdx + itemsPerPage;
              if (endIdx > count) endIdx = count;

              for(int i = startIdx; i < endIdx; i++) {
                  g->lineSprite.setCursor(30, 60 + (i - startIdx) * 25 - drawY);
                  g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
                  g->lineSprite.printf(
                      "%s Lv%d",
                      party.storage[i]->status->getname(),
                      party.storage[i]->status->getLevel()
                  );
              }
          }
          else if (storageMode == 2) {
              g->lineSprite.setCursor(10, 35 - drawY); g->lineSprite.print("誰を預けますか？");
              int count = party.members.size();
              if (count == 0) { g->lineSprite.setCursor(30, 60 - drawY); g->lineSprite.print("誰もいません"); }
              for(int i=0; i<count; i++) {
                  g->lineSprite.setCursor(30, 60 + i*25 - drawY);
                  g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
                  g->lineSprite.printf(
                      "%s Lv%d",
                      party.members[i]->status->getname(),
                      party.members[i]->status->getLevel()
                  );
              }
          }
      } else if (currentMenuState == STATE_SAVE_RESULT) {
        // --- セーブ完了画面 ---
        int x = 60, y = 100 - drawY;
        int w = 200, h = 60;
        
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 30, y + 20);
        if (saveSucceeded) {
          g->lineSprite.print("記録しました");
        }
        else {
          g->lineSprite.print("記録に失敗しました");
        }
      }else if (currentMenuState == STATE_EQUIP_SLOT) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        Status* s = hero.status; // 主人公固定
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("装備 (%s)", s->getname()); 
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        String wName = (s->equipWeaponId == -1) ? "なし" : createItemById(s->equipWeaponId).name;
        String aName = (s->equipArmorId == -1) ? "なし" : createItemById(s->equipArmorId).name;
        String acName = (s->equipAccId == -1) ? "なし" : createItemById(s->equipAccId).name;

        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 60); g->lineSprite.printf("武器: %s", wName.c_str());

        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 90); g->lineSprite.printf("防具: %s", aName.c_str());

        g->lineSprite.setTextColor(listCursor == 2 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 120); g->lineSprite.printf("装飾: %s", acName.c_str());
        
        g->lineSprite.setTextColor(TFT_CYAN);
        g->lineSprite.setCursor(x + 20, y + 160);
        g->lineSprite.printf("ATK:%d DEF:%d", s->getAttack(), s->getDefense());
      }else if (currentMenuState == STATE_EQUIP_LIST) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("装備するアイテム");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        std::vector<int> equippableIndices;
        Item::ItemType targetType = Item::ITEM_WEAPON; // Item::をつける
        if (equipSlot == 1) targetType = Item::ITEM_ARMOR;
        if (equipSlot == 2) targetType = Item::ITEM_ACCESSORY;

        for(int i=0; i<inventory.items.size(); i++) {
          if (inventory.items[i].item.type == targetType) {
            equippableIndices.push_back(i);
          }
        }

        if (equippableIndices.empty()) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("装備できるものがありません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = min(startIdx + itemsPerPage, (int)equippableIndices.size());

          for (int i = startIdx; i < endIdx; i++) {
            int idx = equippableIndices[i];
            Item& it = inventory.items[idx].item;
            
            g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            
            String valStr = "";
            if (it.type == Item::ITEM_WEAPON) valStr = "ATK+" + String(it.value);
            else if (it.type == Item::ITEM_ARMOR) valStr = "DEF+" + String(it.value);
            
            g->lineSprite.printf("%s (%s)", it.name, valStr.c_str());
          }
        }
      }else if (currentMenuState == STATE_INN) {
        int x = 60, y = 80 + yOffset;
        g->lineSprite.fillRect(x, y, 200, 100, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, 200, 100, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 20);
        g->lineSprite.print("一泊 10G です");
        
        g->lineSprite.setCursor(x + 40, y + 50);
        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("泊まる");
        
        g->lineSprite.setCursor(x + 40, y + 75);
        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("やめる");
      }else if (currentMenuState == STATE_SHOP_CHOICE) {
        int x = 60, y = 80 + yOffset;
        g->lineSprite.fillRect(x, y, 200, 120, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, 200, 120, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 10);
        g->lineSprite.print("いらっしゃい");
        
        g->lineSprite.setCursor(x + 40, y + 40);
        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("買う");
        
        g->lineSprite.setCursor(x + 40, y + 65);
        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("売る");

        g->lineSprite.setCursor(x + 40, y + 90);
        g->lineSprite.setTextColor(listCursor == 2 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("出る");
      }else if (currentMenuState == STATE_SHOP_BUY) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("買う (所持金:%dG)", inventory.money);
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        const int shopItems[] = {100, 101, 103, 104, 300, 301, 400, 401, 500};
        int itemCount = shopItemList.size();
        
        // スクロール
        int itemsPerPage = 5;
        int startIdx = 0;
        if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
        int endIdx = min(startIdx + itemsPerPage, itemCount);

        for (int i = startIdx; i < endIdx; i++) {
          // リストからID取得
          int itemId = shopItemList[i];
          Item it = createItemById(itemId);

          g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
          g->lineSprite.printf("%s (%dG)", it.name, it.price);
        }
      }else if (currentMenuState == STATE_SHOP_SELL) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("売る (所持金:%dG)", inventory.money);
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        int itemCount = inventory.items.size();
        if (itemCount == 0) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("売れるものがありません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = min(startIdx + itemsPerPage, itemCount);

          for (int i = startIdx; i < endIdx; i++) {
            Item& it = inventory.items[i].item;
            int sellPrice = it.price / 2;
            
            g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            g->lineSprite.printf("%s x%d (%dG)", it.name, inventory.items[i].quantity, sellPrice);
          }
        }
      }else if (currentMenuState == STATE_CONFIG) {
        int x = 60, y = 40 + yOffset;
        int w = 200, h = 160;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("環境設定");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        // 項目描画ヘルパー
        auto drawItem = [&](int idx, String label, String value) {
          if (listCursor == idx) g->lineSprite.setTextColor(TFT_YELLOW);
          else g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 55 + idx * 25);
          g->lineSprite.print(label);
          g->lineSprite.setCursor(x + 140, y + 55 + idx * 25);
          g->lineSprite.print(value);
        };

        // 1. BGM (引数の config を使用)
        String bgmValStr = String(config.bgmVolume);
        drawItem(0, "BGM", bgmValStr);

        // 2. SE (引数の config を使用)
        String seValStr = String(config.seVolume);
        drawItem(1, "SE", seValStr);

        // 3. 振動 (引数の config を使用)
        String vibValStr = config.vibration ? "ON" : "OFF";
        drawItem(2, "振動", vibValStr);

        // 4. 戻る
        if (listCursor == 3) g->lineSprite.setTextColor(TFT_YELLOW);
        else g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 60, y + 130);
        g->lineSprite.print("戻る");
      }
        
      // ★★★ 画面に転送 ★★★
      g->lineSprite.pushSprite(0, drawY);
    }
    
  }
};

// -------------------------
// BATTLE クラス
// -------------------------
class ImageManager {
private:
  // 64x64画像 x 8枚分 のメモリを確保 (合計 64KB)
  // これが「メモリプール」です。ここ以外には画像を置きません。
  uint16_t* pool[8];

public:
  ImageManager() {
    // 起動時に確保しきる！これでプレイ中の new / delete はゼロになる
    for (int i = 0; i < 8; i++) {
      pool[i] = new uint16_t[64 * 64]; 
      // 黒で塗りつぶしておく
      memset(pool[i], 0, 64 * 64 * 2);
    }
  }

  // 指定したスロット(0-7)に画像を読み込み、そのポインタを返す
  uint16_t* load(int slot, String filename, Sd& sd) {
    if (slot < 0 || slot >= 8) return nullptr;

    // SDカードから、確保済みのメモリプールへ上書きロード
    // (Sdクラスに追加した loadToBuffer を使う)
    if (sd.loadToBuffer(filename, pool[slot])) {
        return pool[slot]; // 成功したらポインタを返す
    } else {
        // 失敗したら黒く塗って返す（エラー落ち防止）
        memset(pool[slot], 0, 64 * 64 * 2);
        return pool[slot];
    }
  }

  // 指定スロットの画像を取得
  uint16_t* get(int slot) {
      if (slot < 0 || slot >= 8) return nullptr;
      return pool[slot];
  }

  // 全消去（実際はメモリを解放せず、黒く塗るだけ）
  // これが「擬似的なガベージコレクション」になります
  void clearAll() {
      for (int i = 0; i < 8; i++) {
          memset(pool[i], 0, 64 * 64 * 2);
      }
  }
};
class Battle {
private:
  // Battle クラス内 (private 推奨)

  // Battle クラス内 (private)

  Status* resolveTarget(Status* originalTarget, Skill& skill) {
    if (originalTarget == nullptr) return nullptr; // 安全対策
    if (originalTarget->getHp() <= 0) return originalTarget;
    
    // 自分・全体・味方への補助技は対象変更しない。
    // RANDOM_ENEMY は各ヒットごとに resolveTarget() を通す。
    if (skill.scope == Skill::TargetScope::SELF ||
        skill.scope == Skill::TargetScope::ALL_ENEMIES ||
        skill.scope == Skill::TargetScope::ALL_ALLIES ||
        skill.type == Skill::EffectType::HEAL ||
        skill.type == Skill::EffectType::REVIVE ||
        skill.type == Skill::EffectType::BUFF)
    {
      return originalTarget;
    }
    // --- 1. 挑発 (Taunt) ---
    Status* tauntTarget = nullptr;
    if (originalTarget->isPlayer()) { // 敵→味方
         if (originalTarget->tauntTurns > 0) return originalTarget;
         if (heroRef && heroRef->status->tauntTurns > 0 && heroRef->status->getHp() > 0) tauntTarget = heroRef->status;
         if (partyRef) {
             for (auto* ally : partyRef->members) {
                 if (ally && ally->status->tauntTurns > 0 && ally->status->getHp() > 0) tauntTarget = ally->status;
             }
         }
    } else { // 味方→敵
         if (originalTarget->tauntTurns > 0) return originalTarget;
         std::vector<Status*> taunters;
         for (auto* enemy : activeEnemies) {
             if (enemy && enemy->status && enemy->status->tauntTurns > 0 && enemy->status->getHp() > 0) {
                 taunters.push_back(enemy->status);
             }
         }
         if (!taunters.empty()) tauntTarget = taunters[random(0, taunters.size())];
    }
    if (tauntTarget != nullptr) return tauntTarget;

    // --- 2. ステルス (Untargetable) ---
    // 狙われた人がステルス中なら、別の生存者に逸らす
    if (originalTarget->untargetableTurns > 0) {
         std::vector<Status*> others;
         if (originalTarget->isPlayer()) {
             // 主人公・仲間から「自分以外」かつ「生存者」を探す
             if (heroRef->status != originalTarget && heroRef->status->getHp() > 0) others.push_back(heroRef->status);
             for (auto* m : partyRef->members) {
                 if (m->status != originalTarget && m->status->getHp() > 0) others.push_back(m->status);
             }
         } else {
             // 敵から「自分以外」かつ「生存者」を探す
             for (auto* e : activeEnemies) {
                 if (e->status != originalTarget && e->status->getHp() > 0) others.push_back(e->status);
             }
         }
         
         // ターゲット変更 (他に誰もいなければそのまま攻撃を受ける)
         if (!others.empty()) {
             return others[random(0, others.size())];
         }
    }

    // --- 3. かばう (Cover) ---
    // 味方限定
    if (originalTarget->isPlayer()) {
         // 誰かが「かばう」発動中か？
         if (heroRef->status != originalTarget && heroRef->status->coverTurns > 0 && heroRef->status->getHp() > 0) return heroRef->status;
         for (auto* ally : partyRef->members) {
             if (ally->status != originalTarget && ally->status->coverTurns > 0 && ally->status->getHp() > 0) return ally->status;
         }
    }

    return originalTarget;
  }

  bool executeHostileSkillOnTarget(
      Skill& skill,
      Status* attacker,
      Status* originalTarget,
      MUSIC& music,
      bool allowTargetRedirect)
  {
    if (attacker == nullptr ||
        originalTarget == nullptr ||
        originalTarget->getHp() <= 0)
    {
      return false;
    }

    // 単体攻撃・ランダム攻撃では
    // 挑発・ステルス・かばうによる対象変更を行う。
    // 全体攻撃では行わない。
    Status* defender = originalTarget;

    if (allowTargetRedirect) {
      defender = resolveTarget(originalTarget, skill);
    }

    if (defender == nullptr ||
        defender->getHp() <= 0)
    {
      return false;
    }

    // 反射は「攻撃」にだけ適用する。
    // 純粋なデバフなど OTHER カテゴリは反射しない。
    bool isOffensive =
        skill.category == Skill::Category::PHYSICAL ||
        skill.category == Skill::Category::MELODY;

    bool isReflected = false;

    if (isOffensive) {
      // キングス・シールド：
      // 有効ターン中は物理・旋律を何度でも反射する
      if (defender->reflectAllTurns > 0) {
        isReflected = true;
      }

      // リフレクト：
      // 物理攻撃を1回だけ反射して、その場で消費する
      else if (defender->reflectPhysicalTurns > 0 &&
              skill.category == Skill::Category::PHYSICAL)
      {
        isReflected = true;
        defender->reflectPhysicalTurns = 0;
      }

      // 旋律反射：
      // 有効ターン中は旋律攻撃を反射する
      // 反射した回数ではなくUpdateTurn()で寿命を減らす
      else if (defender->reflectMagicTurns > 0 &&
              skill.category == Skill::Category::MELODY)
      {
        isReflected = true;
      }
    }

    if (isReflected) {
      music.playSE(7);

      message +=
          "\n" +
          String(defender->getname()) +
          " は跳ね返した！";

      // 反射された場合は使用者自身へ実行
      ExecuteSkill(
          skill,
          attacker,
          attacker,
          music
      );

      return false;
    }

    // カウンター判定のため、攻撃前HPを覚えておく
    int hpBefore = defender->getHp();

    ExecuteSkill(
        skill,
        attacker,
        defender,
        music
    );

    bool tookDamage =
        defender->getHp() < hpBefore;

    // 実際に物理ダメージを受けたときだけ反撃
    if (tookDamage &&
        defender->getHp() > 0 &&
        defender->counterTurns > 0 &&
        skill.category == Skill::Category::PHYSICAL &&
        defender != attacker)
    {
      message +=
          "\n" +
          String(defender->getname()) +
          " の反撃！";

      music.playSE(3);

      int counterDamage = defender->getAttack();
      attacker->takedamage(
          counterDamage,
          Skill::Category::PHYSICAL
      );
      attacker->addPopup(counterDamage, false);
    }

    return defender->getHp() <= 0;
  }

public:
  enum BattleState{
    STATE_TURN_START,     // 0. ターン開始
    STATE_ACTOR_SELECT,     // 1. 行動者を決定
    STATE_COMMAND_SELECT,   // 2. コマンド入力
    STATE_SKILL_SELECT,     // 3. 旋律入力
    STATE_TARGET_SELECT_ENEMY, // 4. ターゲット選択 (敵)
    STATE_TARGET_SELECT_ALLY,  // 5. ターゲット選択 (味方)
    
    // ★★★ 6. (新規) 行動前・継続効果処理 ★★★
    STATE_PRE_ACTION_EFFECTS, 
    
    STATE_ENEMY_AI,         // 7. 敵のAI実行
    STATE_ACTION_READY,     // 8. 行動内容をBattleActionとして確定
    STATE_EXECUTE_NEXT_ACTION, // 9. 
    STATE_ACTION_CALC,      // 10. 行動実行（計算）
    STATE_ACTION_MSG,       // 11. 行動結果のメッセージ
    STATE_CHECK_BATTLE_RESULT, // 12. 勝敗だけ確認
    STATE_END_ACTOR_TURN,      // 13. 現在の行動者のターン終了処理
    STATE_SHOW_XP_GAIN,     // 14. XP獲得表示
    STATE_BATTLE_END,       // 15. 戦闘終了
    STATE_CHECK_STATUS,      // 16. Yボタンステータス
    STATE_SHOW_RESULT_MESSAGE // 17. メッセージ表示
  };
  BattleState currentBattleState;
  static const int CMD_COUNT = 4;
  const char* commands[CMD_COUNT] = {"たたかう", "にげる", "せんりつ", "ぼうぎょ"};

  // UIカーソルは用途ごとに分離
  int commandSelected = 0;
  int skillSelected = 0;

  enum class BattleEndReason {
    NONE,
    VICTORY,
    DEFEAT,
    ESCAPE
  };

  BattleEndReason battleEndReason = BattleEndReason::NONE;

  enum class BattlePhase {
    PLANNING,   // 行動を決めている段階
    EXECUTING   // 決めた行動を実行している段階
  };

  BattlePhase battlePhase = BattlePhase::PLANNING;

  bool inBattle = false;
  bool heroLeveledUp = false;
  bool isBossBattleMode = false;

  enum class ActionType {
    NONE,
    ATTACK,
    SKILL,
    DEFEND,
    FLEE
  };

  enum class ActionPriority {
    NORMAL = 0,
    FIRST_STRIKE = 1
  };

  struct BattleAction {
    Status* actor = nullptr;          // 行動するキャラ
    ActionType type = ActionType::NONE;
    int skillIndex = -1;             // SKILLの場合のみ使用
    Status* target = nullptr;         // 単体対象。全体技などではnullptr可

    ActionPriority priority = ActionPriority::NORMAL;
    int speedSnapshot = 0;           // 行動決定時点のSPD
  };

  enum class MessageFlow {
    ACTION_COMPLETE, // 実際の行動が終わった
    CONTINUE_TURN,   // 毒・回復等の表示後、まだ行動する
    END_TURN         // 死亡・麻痺等でこの人の行動を終了
  };

  ActionType pendingActionType = ActionType::NONE;
  int pendingSkillIndex = -1;

  MessageFlow messageFlow = MessageFlow::ACTION_COMPLETE;

  int enemyTargetIndex = 0;   // 敵リスト(activeEnemies)のカーソル (targetIndexから変更)
  int allyTargetIndex = 0;    // 味方リストのカーソル (新規)
  Status* currentTarget = nullptr; // ターゲットのStatus

  String message = "";
  //Enemy* currentEnemy = nullptr;
  std::vector<Enemy*> activeEnemies;
  Caractor* heroRef = nullptr; // 戦闘中の主人公への参照
  Party* partyRef = nullptr;   // 戦闘中の仲間パーティーへの参照

  //bool shouldReturncs = false;
  //bool iscaclated = false;
  //int lastGainedXP = 0;

  // ★★★ 行動順管理リスト ★★★
  std::vector<Status*> actorList; // 戦闘参加者 (Speed順)
  int currentActorIndex = 0;        // 現在行動中のキャラのインデックス
  Status* currentActor = nullptr; // 現在行動中のキャラのStatus
  // 1ターン分の行動予定
  std::vector<BattleAction> plannedActions;
  BattleAction currentBattleAction;
  int currentPlannedActionIndex = 0;
  int lastDamage = 0;             // 直前のダメージ計算結果
  String lastActionMessage = "";
  int lastGainedXP = 0;
  String lastEvolvedName = ""; // ★★★ これを追加 ★★★
  int turnCount = 1;
  int statusPage = 0;
  //std::vector<uint16_t*> enemyImages; // 敵の画像 (64x64)
  //std::vector<uint16_t*> allyImages;  // 味方の画像 (64x64)
  uint16_t* activeImages[8];
  std::vector<String> resultMessages;
  int resultMessageIndex = 0;

  BattleAction createBattleAction(
      Status* actor,
      ActionType type,
      int skillIndex = -1,
      Status* target = nullptr)
  {
    BattleAction action;

    action.actor = actor;
    action.type = type;
    action.skillIndex = skillIndex;
    action.target = target;

    if (actor != nullptr) {
      action.speedSnapshot = actor->getSpeed();
    }

    // 先制技判定
    if (type == ActionType::SKILL && actor != nullptr) {
      std::vector<Skill>& skills = actor->getLearnedSkills();

      if (skillIndex >= 0 &&
          skillIndex < static_cast<int>(skills.size()))
      {
        int skillId = skills[skillIndex].id;

        // 必ず先制
        if (skillId == 38 ||   // ソニックブレイド
            skillId == 84 ||   // マッハブレイド
            skillId == 121)    // ソニックダガー
        {
          action.priority = ActionPriority::FIRST_STRIKE;
        }
      }
    }

    return action;
  }

  Battle() {
    activeEnemies.reserve(4);
    plannedActions.reserve(8);

    for(int i=0; i<8; i++) {
      activeImages[i] = nullptr;
    }
  }

  void start(MUSIC &music, Sd &sd, Caractor& hero, Party& party, int floor, ImageManager &imgMgr, Map& map) {
    if (inBattle) return;
    inBattle = true;
    message = "敵が現れた！";
    isBossBattleMode = false; 
    battleEndReason = BattleEndReason::NONE;
    commandSelected = 0;
    skillSelected = 0;
    turnCount = 1;
    heroLeveledUp = false;
    // ★★★ 変数の完全リセット (重要) ★★★
    pendingActionType = ActionType::NONE;
    pendingSkillIndex = -1;
    messageFlow = MessageFlow::ACTION_COMPLETE;
    currentTarget = nullptr;
    currentActor = nullptr;
    currentActorIndex = 0;
    lastDamage = 0;
    lastActionMessage = "";
    enemyTargetIndex = 0;
    allyTargetIndex = 0;
    plannedActions.clear();
    currentBattleAction = BattleAction();
    battlePhase = BattlePhase::PLANNING;
    currentPlannedActionIndex = 0;

    // 参加メンバーへの参照を保存
    heroRef = &hero;
    partyRef = &party;
    // ★★★★★★★★★★★★★★★★★★★★★
    for (auto enemy : activeEnemies) {
      if (enemy) delete enemy;
    }
    activeEnemies.clear();
    activeEnemies.reserve(4);
    if (floor >= 10 &&
        floor <= 50 &&
        floor % 10 == 0 &&
        !map.isTown)
    {
      isBossBattleMode = true;
      int bossId = 0;
      if (floor == 10) bossId = 80;
      else if (floor == 20) bossId = 81;
      else if (floor == 30) bossId = 82;
      else if (floor == 40) bossId = 83;
      else if (floor == 50) bossId = 84;

      Enemy* boss = new Enemy(bossId, 1);
      if (boss->status) boss->name = boss->status->getname();
      activeEnemies.push_back(boss);
      critical_section_enter_blocking(&sdLock);
      String imgName = "/e_" + String(bossId) + ".bin";
      activeImages[0] = imgMgr.load(0, imgName, sd);

      // 他のスロットは空にする
      for(int i=1; i<4; i++) activeImages[i] = nullptr;
      critical_section_exit(&sdLock);

      // 3. ボス戦BGM (ID 3:ボス, ID 4:ラスボス)
      int bgmId = (floor == 50) ? 4 : 3;
      music.switchTrack(bgmId, sd);
    }else{
      std::vector<int> candidates;
      bool rare_36_appears = false; 
      bool rare_37_appears = false;
      if (floor <= 10) {
        // 0~4 (基本) + 22 (オーバーロード:強敵)
        candidates = {0, 1, 2, 3, 4}; 
      }else if (floor <= 20) {
        // 5~9 (1段階) + 22 (オーバーロード)
        candidates = {5, 6, 7, 8, 9};
      }else if (floor <= 30) {
        // 10~14 (2段階)
        candidates = {10, 11, 12, 13, 14};
        rare_36_appears = true; // 21階〜: はぐれノイズ解禁
      }else if (floor <= 40) {
        // 15, 17, 19, 20, 21 (3段階) + 24 (ゼロ・ブレード)
        // ※ 除外: 16(スワッシュ), 18(イージス)
        candidates = {15, 17, 19, 20, 21};
        rare_36_appears = true;
        rare_37_appears = true; // 31階〜: ノイズ・キング解禁
      }else { // 41階〜50階 (それ以降含む)
        // 22, 24, 26, 28, 30, 32, 34 (4段階)
        // ※ 除外: 23, 25, 27, 29, 31, 33
        candidates = {22, 24, 26, 28, 30, 32, 34};
        rare_36_appears = true;
        rare_37_appears = true;
      }
      int enemyCount = random(1, 5);
      for (int i = 0; i < enemyCount; i++) {
        int who = 0;
        int r = random(100);
        
        // 1. まずレア敵の抽選 (メタルスライム枠)
        if (rare_37_appears && r < 3) { // 3%
          who = 37; // ノイズ・キング
        } else if (rare_36_appears && r < 8) { // 5%
          who = 36; // はぐれノイズ
        } else if (floor <= 20 && random(100) < 3) { 
          who = 22; // オーバーロード出現！(2%の確率)
        }else if(floor <= 40 && floor > 20 && random(100) < 3) {
          who = 24;
        }
        // 2. 通常敵の抽選 (リストからランダム)
        else {
          if (!candidates.empty()) {
            int idx = random(0, candidates.size());
            who = candidates[idx];
          } else {
            who = 0; // 万が一リストが空ならストライカー
          }
        }
        
        // 敵の生成
        Enemy* newEnemy = new Enemy(who, floor);
      
        if (newEnemy->status) {
          newEnemy->status->resetBattleStats();
          newEnemy->name = newEnemy->status->getname();
        }
        activeEnemies.push_back(newEnemy);
      }
      
      // 画像ロード処理
      critical_section_enter_blocking(&sdLock);
      for (int i = 0; i < 4; i++) {
        if (i < activeEnemies.size()) {
          String fname = "/e_" + String(activeEnemies[i]->who) + ".bin";
          activeImages[i] = imgMgr.load(i, fname, sd);
        } else {
          activeImages[i] = nullptr;
        }
      }
      critical_section_exit(&sdLock);
      
      music.switchTrack(2, sd);
    }
    
    critical_section_enter_blocking(&sdLock);
    activeImages[4] = imgMgr.load(4, "/a_38.bin", sd);
    for (int i = 0; i < party.members.size(); i++) {
      if (i < 3) {
        int id = party.members[i]->status->getWho();
        if (id == 99) id = 38; // デバッグ用

        String fname = "/a_" + String(id) + ".bin";
        activeImages[5 + i] = imgMgr.load(5 + i, fname, sd);
      }
    }
    critical_section_exit(&sdLock);
    // ★ 行動順リストを作成 (ソートはしない)
    actorList.clear();
    actorList.push_back(hero.status);
    for (auto ally : party.members) {
      actorList.push_back(ally->status);
    }
    // ★ 敵全員を行動順リストに追加
    for (auto enemy : activeEnemies) {
      actorList.push_back(enemy->status);
    }
    
    // ★ ターン開始処理へ
    currentBattleState = STATE_TURN_START;
  }

  void update(Controller &ctrl, GameState &state, MUSIC &music, Sd &sd, Caractor &hero, Inventory &inventory, Vibration &vibration, Map &map, Graphic &g, bool* bossFlags) {
    if (!inBattle) return;
    Party& party = *partyRef; 

    bool aPressed = ctrl.pressedDebounced(ctrl.BTN_A);
    bool bPressed = ctrl.pressedDebounced(ctrl.BTN_B);
    bool yPressed = ctrl.pressedDebounced(ctrl.BTN_Y);
    bool upPressed = ctrl.pressedDebounced(ctrl.BTN_UP);
    bool downPressed = ctrl.pressedDebounced(ctrl.BTN_DOWN);
    bool rightPressed = ctrl.pressedDebounced(ctrl.BTN_RIGHT);
    bool leftPressed = ctrl.pressedDebounced(ctrl.BTN_LEFT);

    // --- Yボタン（ステータス確認）の最優先処理 ---
    if (yPressed) {
      if (currentBattleState == STATE_COMMAND_SELECT) {
        music.playSE(2);
        currentBattleState = STATE_CHECK_STATUS;
        statusPage = 0; // ★ 最初は主人公(0)から
      }
    } else if (currentBattleState == STATE_CHECK_STATUS) {
      // ★ 左右でページ切り替え
      int totalMembers = 1 + party.members.size(); // 主人公 + 仲間数
      
      if (leftPressed) {
          statusPage = (statusPage - 1 + totalMembers) % totalMembers;
      }
      if (rightPressed) {
          statusPage = (statusPage + 1) % totalMembers;
      }

      if (aPressed || bPressed || yPressed) {
        music.playSE(1);
        currentBattleState = STATE_COMMAND_SELECT;
      }
      return; 
    }

    // --- メインの戦闘ステートマシン ---
    switch(currentBattleState){

      // ★★★ 0. ターン開始処理 ★★★ (変更なし)
      case STATE_TURN_START:
        message = "ターン " + String(turnCount);

        if (aPressed) {

          // -------------------------
          // 新しいターンの行動予約を開始
          // -------------------------
          battlePhase = BattlePhase::PLANNING;

          plannedActions.clear();
          currentPlannedActionIndex = 0;

          currentActorIndex = 0;
          currentActor = nullptr;
          currentBattleAction = BattleAction();

          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;

          currentBattleState = STATE_ACTOR_SELECT;
        }
        break;

      // ★★★ 1. 行動者を決定する (変更なし)
      case STATE_ACTOR_SELECT: {
        // -------------------------
        // 全員分の行動予約が完了した
        // -------------------------
        if (currentActorIndex >= actorList.size()) {

          // 先制技を最優先、その中ではSPD順
          std::stable_sort(
            plannedActions.begin(),
            plannedActions.end(),
            [](const BattleAction& a, const BattleAction& b) {

              if (a.priority != b.priority) {
                return static_cast<int>(a.priority) >
                      static_cast<int>(b.priority);
              }

              return a.speedSnapshot > b.speedSnapshot;
            }
          );

          battlePhase = BattlePhase::EXECUTING;
          currentPlannedActionIndex = 0;

          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // -------------------------
        // 次に行動を決めるキャラ
        // -------------------------
        currentActor = actorList[currentActorIndex];

        if (currentActor == nullptr) {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // 死亡中なら行動予約しない
        if (currentActor->getHp() <= 0) {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // 前のキャラの行動内容を残さない
        pendingActionType = ActionType::NONE;
        pendingSkillIndex = -1;
        currentTarget = nullptr;

        // ★ここでは状態異常処理をしない
        // 実際に行動する直前に PRE_ACTION_EFFECTS を通す

        if (currentActor->isPlayer()) {
          currentBattleState = STATE_COMMAND_SELECT;
        }
        else {
          currentBattleState = STATE_ENEMY_AI;
        }

        break;
      }

      // 2. 味方のコマンド入力
      case STATE_COMMAND_SELECT: 
        message = String(currentActor->getname()) + "：コマンドを選ぼう！";
        if (leftPressed) {commandSelected = (commandSelected - 1 + CMD_COUNT) % CMD_COUNT; music.playSE(2);}
        if (upPressed)   {commandSelected = (commandSelected - 2 + CMD_COUNT) % CMD_COUNT; music.playSE(2);}
        if (rightPressed){commandSelected = (commandSelected + 1) % CMD_COUNT; music.playSE(2);}
        if (downPressed) {commandSelected = (commandSelected + 2) % CMD_COUNT; music.playSE(2);}

        if (aPressed) {
          if (commandSelected == 0) { // たたかう
            music.playSE(0);
            pendingActionType = ActionType::ATTACK;
            pendingSkillIndex = -1;
            enemyTargetIndex = 0;   // ★ 敵カーソルをリセット
            currentBattleState = STATE_TARGET_SELECT_ENEMY; // ★ 敵選択へ
          } 
          else if (commandSelected == 1) { // にげる
            music.playSE(0);
            message = "逃げ出した！";
            battleEndReason = BattleEndReason::ESCAPE;
            currentBattleState = STATE_BATTLE_END;
          } 
          else if (commandSelected == 2) { // せんりつ
            if (currentActor->getSilencedTurns() > 0) {
              music.playSE(1);
              message = String(currentActor->getname()) + " は沈黙している！";
              // (Aボタン押しっぱなしループを防ぐ)
              if(ctrl.pressed(ctrl.BTN_A)) delay(200);
            } else {
              music.playSE(0);
              skillSelected = 0;
              currentBattleState = STATE_SKILL_SELECT;
            }
          }
          else if (commandSelected == 3) {
            music.playSE(0);
            pendingActionType = ActionType::DEFEND;
            pendingSkillIndex = -1;
            
            // ターゲット選択は不要なので、行動確定へ
            currentTarget = currentActor;
            currentBattleState = STATE_ACTION_READY;
          }
        }
        break;

      
      // 3. 旋律（せんりつ）選択
      case STATE_SKILL_SELECT: {
        int skillCount = currentActor->getLearnedSkills().size();
        if (skillCount == 0) {
          message = "覚えている旋律がない！";
          if(aPressed) currentBattleState = STATE_COMMAND_SELECT;
          break;
        }
        
        message = "どの旋律を使う？";
        if (upPressed) {
          skillSelected = (skillSelected - 1 + skillCount) % skillCount;
          music.playSE(2);
        }
        if (downPressed) {
          skillSelected = (skillSelected + 1) % skillCount;
          music.playSE(2);
        }

        if (aPressed) {
          Skill& skill = currentActor->getLearnedSkills()[skillSelected];
          
          if (currentActor->getTp() >= skill.tpCost) {
            music.playSE(0);

            pendingActionType = ActionType::SKILL;
            pendingSkillIndex = skillSelected;
            
            // ★★★ スキルの対象範囲(scope)によって分岐 ★★★
            switch (skill.scope) {
              case Skill::TargetScope::SELF:
                currentTarget = currentActor;
                currentBattleState = STATE_ACTION_READY;
                break;
              case Skill::TargetScope::SINGLE_ENEMY:
                enemyTargetIndex = 0; // 敵カーソルをリセット
                currentBattleState = STATE_TARGET_SELECT_ENEMY;
                break;
              case Skill::TargetScope::SINGLE_ALLY:
                allyTargetIndex = 0; // 味方カーソルをリセット
                currentBattleState = STATE_TARGET_SELECT_ALLY;
                break;
              case Skill::TargetScope::ALL_ENEMIES:
              case Skill::TargetScope::ALL_ALLIES:
              case Skill::TargetScope::RANDOM_ENEMY:
                currentTarget = nullptr;
                currentBattleState = STATE_ACTION_READY;
                break;
            }
          } else {
            music.playSE(1);
            message = "TPが足りない！";
            // (Aボタン押しっぱなしでTP不足ループするのを防ぐ)
            if(ctrl.pressed(ctrl.BTN_A)) delay(200);
            currentBattleState = STATE_COMMAND_SELECT;
          }
        }
        // Bボタンでコマンド選択に戻る
        if (bPressed) {
          music.playSE(1);
          currentBattleState = STATE_COMMAND_SELECT;
        }
        break;
      }

      // ★★★ 4. ターゲット選択 (敵) ★★★
      case STATE_TARGET_SELECT_ENEMY: {
        message = "誰を狙う？";
        while (activeEnemies[enemyTargetIndex]->status->getHp() <= 0){
          enemyTargetIndex = (enemyTargetIndex - 1 + activeEnemies.size()) % activeEnemies.size();
        }
        // 生きている敵だけを選べるように
        if (leftPressed) {
          do {
            enemyTargetIndex = (enemyTargetIndex - 1 + activeEnemies.size()) % activeEnemies.size();
          } while (activeEnemies[enemyTargetIndex]->status->getHp() <= 0);
          music.playSE(2);
        }
        if (rightPressed) {
          do {
            enemyTargetIndex = (enemyTargetIndex + 1) % activeEnemies.size();
          } while (activeEnemies[enemyTargetIndex]->status->getHp() <= 0);
          music.playSE(2);
        }

        // Aボタンで決定
        if (aPressed) {
          music.playSE(0);
          currentTarget = activeEnemies[enemyTargetIndex]->status;
          currentBattleState = STATE_ACTION_READY;
        }
        // Bボタンでキャンセル
        if (bPressed) {
          music.playSE(1);
          if (pendingActionType == ActionType::SKILL) {
            currentBattleState = STATE_SKILL_SELECT;
          } else {
            currentBattleState = STATE_COMMAND_SELECT;
          }
        }
        break;
      }
      
      // ★★★ 5. ターゲット選択 (味方) (新規) ★★★
      case STATE_TARGET_SELECT_ALLY: {
        message = "誰にかける？";

        bool isReviveSkill = false;

        if (pendingActionType == ActionType::SKILL) {
          std::vector<Skill>& skills = currentActor->getLearnedSkills();

          if (pendingSkillIndex >= 0 &&
              pendingSkillIndex < static_cast<int>(skills.size())) {
            isReviveSkill =
                (skills[pendingSkillIndex].type ==
                Skill::EffectType::REVIVE);
          }
        }

        std::vector<Status*> allies =
            isReviveSkill ? getDeadAllies()
                          : getAliveAllies();

        if (allies.empty()) {
          if (isReviveSkill) {
            message = "倒れている味方がいない！";
          }

          currentBattleState = STATE_SKILL_SELECT;
          break;
        }

        if (upPressed) {
          allyTargetIndex = (allyTargetIndex - 1 + allies.size()) % allies.size();
          music.playSE(2);
        }
        if (downPressed) {
          allyTargetIndex = (allyTargetIndex + 1) % allies.size();
          music.playSE(2);
        }

        if (aPressed) {
          music.playSE(0);
          currentTarget = allies[allyTargetIndex];
          currentBattleState = STATE_ACTION_READY;
        }
        // Bボタンで旋律選択に戻る
        if (bPressed) {
          music.playSE(1);
          currentBattleState = STATE_SKILL_SELECT;
        }
        break;
      }

      // ★★★ 6. (新規) 行動前・継続効果処理 ★★★
      case STATE_PRE_ACTION_EFFECTS: {
        message = "";
        lastDamage = 0;
        
        // 1. ターン消費と、毒・リジェネの効果を適用
        lastDamage = currentActor->ApplyTurnEffects();

        if (lastDamage > 0) { // 毒ダメージ
          music.playSE(3);
          message = String(currentActor->getname()) + " は毒:" + String(lastDamage) + " のダメージ！";
        } else if (lastDamage < 0) { // HPリジェネ
          music.playSE(7);
          message = String(currentActor->getname()) + " は回復:" + String(-lastDamage) + " 回復！";
        }
        if (currentActor->getHp() <= 0) {
          // ★★★ オートリバイブ判定 (追加) ★★★
          if (currentActor->autoReviveTurns > 0) {
            currentActor->autoReviveTurns = 0; // 効果消費
            currentActor->Revive(50); // HP半分で復活
            music.playSE(7);
            message += "\n" + String(currentActor->getname()) + " は自動蘇生した！";
          }else{
            message = String(currentActor->getname()) + " は力尽きた…";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }
        }

        // 3. 強制行動不能チェック
        if (currentActor->cantMoveTurns > 0) {
          if (message != "") {
            message += "\n";
          }

          message += String(currentActor->getname()) + " は動けない！";

          currentActor->cantMoveTurns--;

          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // 4. 麻痺(Paralyzed)チェック (isParalyzed -> getParalyzedTurns)
        if (currentActor->getParalyzedTurns() > 0) {
          if (random(100) < 50) { // (例: 50%で行動不能)
            message = String(currentActor->getname()) + " は痺れて動けない！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }
        }
        
        // 5. 混乱(Confused)チェック
        if (currentActor->getConfusedTurns() > 0) {
          message = String(currentActor->getname()) + " は混乱している！";
          // (例: 50%で暴走)
          if (random(100) < 50) {
              message += "\nわけがわからず攻撃！";
              // pendingAction = 0;
              pendingActionType = ActionType::ATTACK;
              pendingSkillIndex = -1;
              
              // ターゲットを敵味方全員からランダムで選択
              std::vector<Status*> allTargets = getAliveAllies(); // 味方リスト
              for (auto enemy : activeEnemies) { // 敵リスト
                  if(enemy->status->getHp() > 0) allTargets.push_back(enemy->status);
              }
              
              if (allTargets.empty()) {
                currentTarget = nullptr;
              } else {
                currentTarget = allTargets[random(0, allTargets.size())];
              }

              currentBattleState = STATE_ACTION_READY;
              break;
          }
          // (暴走しなかった場合は、メッセージ表示(Aボタン待ち)へ)
        }

        // 6. メッセージがあるか？
        if (message != "") { // 毒/回復/混乱(不発)のメッセージがあった
          messageFlow = MessageFlow::CONTINUE_TURN;
          currentBattleState = STATE_ACTION_MSG;
        } else {
          // 継続効果が何もなければ、通常の行動へ
          if (battlePhase == BattlePhase::EXECUTING) {
            // すでに行動内容は決まっているので、そのまま実行
            currentBattleState = STATE_ACTION_CALC;
          }
          else {
            // まだ行動を決める段階
            if (currentActor->isPlayer()) {
              currentBattleState = STATE_COMMAND_SELECT;
            } else {
              currentBattleState = STATE_ENEMY_AI;
            }
          }
        }
        break;
      }

      // 7. 敵のAI実行
      case STATE_ENEMY_AI: {
        String enemyName = currentActor->getname();
        message = enemyName + " の攻撃!";
        // pendingAction = 0; // デフォルトは通常攻撃
        pendingActionType = ActionType::ATTACK;
        pendingSkillIndex = -1;
        
        bool willFlee = false;
        // はぐれノイズ(36): 50%で逃げる
        int who = currentActor->getWho();
        if (who == 36 && random(100) < 30) willFlee = true;
        
        // ノイズ・キング(37): 70%で逃げる (より逃げやすい)
        if (who == 37 && random(100) < 40) willFlee = true;
        if (willFlee) {
          pendingActionType = ActionType::FLEE;
          pendingSkillIndex = -1;
          message = enemyName + " は逃げ出した！";
          currentBattleState = STATE_ACTION_READY;
          break;
        }
        // 1. デフォルトターゲット (プレイヤー側からランダム)
        std::vector<Status*> playerSide = getAliveAllies();
        if (playerSide.empty()) currentTarget = nullptr; 
        else currentTarget = playerSide[random(0, playerSide.size())];

        // 2. スキル選択ロジック
        std::vector<Skill>& skills = currentActor->getLearnedSkills();
        
        if (!skills.empty() && currentActor->getSilencedTurns() <= 0) {
          int who = currentActor->getWho();
          int curTP = currentActor->getTp();
          int curHP = currentActor->getHp();
          int maxHP = currentActor->getMaxHp();
          int selectedSkillIndex = -1; 

          // [ヘルパー] IDでスキルを探す
          auto findSkillIndex = [&](int id) -> int {
            for(int i=0; i<skills.size(); i++) {
              if(skills[i].id == id && skills[i].tpCost <= curTP) return i;
            }
            return -1;
          };

          // --- 特殊AI ---
          if (who == 80) { // スプラウト・ギア
            if (curHP < maxHP / 2 && random(100) < 50) selectedSkillIndex = findSkillIndex(47); 
            else if (random(100) < 30) selectedSkillIndex = findSkillIndex(21); 
          }
          else if (who == 81) { // ソレイユ
            if (curHP < maxHP / 2) { 
              if (random(100) < 60) selectedSkillIndex = findSkillIndex(34); 
              else if (random(100) < 40) selectedSkillIndex = findSkillIndex(87); 
            } else { 
              if (random(100) < 50) selectedSkillIndex = findSkillIndex(52); 
            }
          }
          else if (who == 82) { // ハーベスト・スティンガー
            if (currentTarget && currentTarget->poisonTurns > 0) selectedSkillIndex = findSkillIndex(42);
            else if (random(100) < 70) {
              int r = random(3);
              if (r==0) selectedSkillIndex = findSkillIndex(105); 
              else if(r==1) selectedSkillIndex = findSkillIndex(45); 
              else selectedSkillIndex = findSkillIndex(113); 
            }
          }
          else if (who == 83) { // 雪影
            if (curHP < maxHP / 3) selectedSkillIndex = findSkillIndex(123); 
            else if (turnCount % 3 == 0) selectedSkillIndex = findSkillIndex(random(100)<50 ? 39 : 73);
          }
          else if (who == 84) { // ラスボス
            if (curHP < maxHP / 4) selectedSkillIndex = findSkillIndex(127); 
            else if (turnCount % 4 == 0) selectedSkillIndex = findSkillIndex(114); 
            else if (random(100) < 30) selectedSkillIndex = findSkillIndex(86);
          }
          // --- 回復役AI ---
          else if (who == 3 ||
                  who == 8 ||
                  who == 13 ||
                  who == 20 ||
                  who == 32)
          {
            // HPが半分未満の生存中の味方を探す
            Status* injuredTarget = nullptr;

            for (auto e : activeEnemies) {
              if (e != nullptr &&
                  e->status != nullptr &&
                  e->status->getHp() > 0 &&
                  e->status->getHp() < e->status->getMaxHp() / 2)
              {
                injuredTarget = e->status;
                break;
              }
            }

            // 回復が必要な味方がいる場合だけ回復技を探す
            if (injuredTarget != nullptr) {

              int healIdx = -1;
              int bestHealPower = -1;

              for (int i = 0;
                  i < static_cast<int>(skills.size());
                  i++)
              {
                const Skill& skill = skills[i];

                // 使用可能な回復スキルだけを対象にする
                if (skill.type == Skill::EffectType::HEAL &&
                    skill.tpCost <= curTP)
                {
                  // 複数ある場合は威力が高いものを使う
                  if (skill.power > bestHealPower) {
                    bestHealPower = skill.power;
                    healIdx = i;
                  }
                }
              }

              if (healIdx != -1) {
                selectedSkillIndex = healIdx;
                currentTarget = injuredTarget;
              }
            }
          }

          // --- 汎用AI ---
          if (selectedSkillIndex == -1 && random(100) < 30) {
            std::vector<int> usableIndices;
            for(int i=0; i<skills.size(); i++) {
              if (skills[i].tpCost <= curTP) usableIndices.push_back(i);
            }
            if (!usableIndices.empty()) {
              selectedSkillIndex = usableIndices[random(0, usableIndices.size())];
            }
          }

          // --- 行動確定後のターゲット調整 ---
          if (selectedSkillIndex != -1) {
            pendingActionType = ActionType::SKILL;
            pendingSkillIndex = selectedSkillIndex;
            Skill& s = skills[selectedSkillIndex];
            message = enemyName + " の " + s.name + "!";
            
            // ★★★ 重要: スキル範囲によるターゲットの強制変更 ★★★
            
            // 自分のみ
            if (s.scope == Skill::TargetScope::SELF) {
              currentTarget = currentActor;
            }
            // 味方単体（敵から見た味方＝他の敵）
            else if (s.scope == Skill::TargetScope::SINGLE_ALLY) {
              // もしターゲットが「プレイヤー側」になっていたら、「敵側」に切り替える
              if (currentTarget == nullptr || currentTarget->isPlayer()) {
                // 生きている敵リストを作成
                std::vector<Status*> livingEnemies;
                for(auto e : activeEnemies) if(e->status->getHp() > 0) livingEnemies.push_back(e->status);
                
                if (!livingEnemies.empty()) {
                  // ランダムな仲間にかける（回復AIですでに決まっている場合は上書きしないよう注意が必要だが、
                  // ここでは「回復AI以外で選ばれた補助魔法」を想定してランダムにする）
                  // ※回復AIでcurrentTargetを変えた場合、isPlayer()はfalseなのでここには入らない。完璧。
                  currentTarget = livingEnemies[random(0, livingEnemies.size())];
                } else {
                  currentTarget = currentActor; // 誰もいなければ自分
                }
              }
            }
            // (全体攻撃やランダム攻撃の場合、STATE_ACTION_CALC でループ処理されるので currentTarget は無視でOK)
          }
        }
        
        currentBattleState = STATE_ACTION_READY;
        break;
      }

      // 8. 
      case STATE_ACTION_READY: {
        currentBattleAction = createBattleAction(
            currentActor,
            pendingActionType,
            pendingSkillIndex,
            currentTarget
        );

        // -------------------------
        // 行動予約フェーズ
        // -------------------------
        if (battlePhase == BattlePhase::PLANNING) {

          plannedActions.push_back(currentBattleAction);

          // 次のキャラへ
          currentActorIndex++;

          // 前の行動情報をクリア
          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;
          currentBattleAction = BattleAction();

          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // -------------------------
        // 実行フェーズ
        // -------------------------
        // 混乱などによって実行直前に行動内容が変更された場合は
        // 新たに予約せず、そのまま実行する。
        currentBattleState = STATE_ACTION_CALC;

        break;
      }

      // 9. 
      case STATE_EXECUTE_NEXT_ACTION: {
        // 全ての予約行動を実行し終えた
        if (currentPlannedActionIndex >= plannedActions.size()) {
          // 今のターンの全行動が終了したので、
          // 次のターン番号へ進める
          turnCount++;

          battlePhase = BattlePhase::PLANNING;
          currentBattleState = STATE_TURN_START;
          break;
        }

        currentBattleAction =
            plannedActions[currentPlannedActionIndex];

        currentActor = currentBattleAction.actor;

        // nullptrならスキップ
        if (currentActor == nullptr) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // ★このキャラの実際の行動順が来たので、
        // 前回の「ぼうぎょ」をここで解除する
        currentActor->isDefending = false;

        // 行動前に倒されていたキャラはスキップ
        if (currentActor->getHp() <= 0) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // 予約していた行動を既存実行系へ戻す
        pendingActionType = currentBattleAction.type;
        pendingSkillIndex = currentBattleAction.skillIndex;
        currentTarget = currentBattleAction.target;

        // 毒・麻痺・混乱などはここから処理する
        currentBattleState = STATE_PRE_ACTION_EFFECTS;

        break;
      }

      // 10. 行動の実行（計算）
      case STATE_ACTION_CALC: {
        lastDamage = 0;
        lastActionMessage = "";

        messageFlow = MessageFlow::ACTION_COMPLETE;
        // ★★★ 修正A: フリーズ防止 (ポインタチェックを追加) ★★★
        if (heroRef && heroRef->status) heroRef->status->clearPopups();
        if (partyRef) {
            for(auto m : partyRef->members) if(m && m->status) m->status->clearPopups();
        }
        for(auto e : activeEnemies) if(e && e->status) e->status->clearPopups();

        if (pendingActionType == ActionType::FLEE) {
          // 1. 経験値とドロップを0にする (倒した扱いにはしない)
          // (StatusからはEnemyオブジェクトに辿れないので、リストから探す)
          for (auto e : activeEnemies) {
            if (e->status == currentActor) {
              e->xpYield = 0;
              e->dropItemId = 0; 
              break;
            }
          }

          // 2. HPを0にして強制退場させる
          // (ダメージ処理ではなく、直接0を代入)
          currentActor->setHp(0); 

          // 3. 効果音 (逃走音)
          music.playSE(3); // シュッという音など

          // 4. メッセージ表示へ
          messageFlow = MessageFlow::ACTION_COMPLETE;
          currentBattleState = STATE_ACTION_MSG;
          break; 
        }
        if (pendingActionType == ActionType::DEFEND) { // 防御
          // 1. 防御フラグを立てる
          currentActor->isDefending = true;

          // 2. 音とメッセージ
          music.playSE(5); // 何か適当な音（物理防御音など）
          message = String(currentActor->getname()) + " は身を守っている！";
          
          // 3. メッセージ表示ステートへ
          messageFlow = MessageFlow::ACTION_COMPLETE;
          currentBattleState = STATE_ACTION_MSG;
          break; 
        }
        // 行動者がいない場合はスキップ
        if (!currentActor) {
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // スキル情報取得
        Skill currentSkill(0);

        // 実行時に予約対象を変更したか
        bool targetChanged = false;

        // 行動予約後に沈黙を受けている可能性があるため、
        // 実行時にもスキル使用可否を確認する
        if (pendingActionType == ActionType::SKILL &&
            currentActor->getSilencedTurns() > 0)
        {
          message =
              String(currentActor->getname()) +
              " は沈黙していて旋律を使えない！";

          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        if (pendingActionType == ActionType::ATTACK) {
          currentSkill = Skill(0);
          currentSkill.name = "攻撃";
          currentSkill.power = 50;
          currentSkill.type = Skill::EffectType::DAMAGE;
          currentSkill.scope = Skill::TargetScope::SINGLE_ENEMY;
          currentSkill.dependence = Skill::StatDependence::ATK;
          currentSkill.category = Skill::Category::PHYSICAL;

          // 予約していた攻撃対象がすでに倒れていた場合
          if (currentTarget == nullptr ||
              currentTarget->getHp() <= 0)
          {
            std::vector<Status*> candidates =
                currentActor->isPlayer()
                    ? getAliveEnemies()
                    : getAliveAllies();

            if (candidates.empty()) {
              message =
                  String(currentActor->getname()) +
                  " の攻撃！\n対象がいない！";

              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          message = String(currentActor->getname()) + " の攻撃！";

          if (targetChanged) {
            message += "\n対象を変更した！";
          }
        }
        else if (pendingActionType == ActionType::SKILL) {
          std::vector<Skill>& skills = currentActor->getLearnedSkills();

          if (pendingSkillIndex < 0 ||
            pendingSkillIndex >= skills.size()) {

            message = "スキル指定エラー";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          currentSkill = skills[pendingSkillIndex];

          // --- 実行時ターゲット再確認 ---

          // 単体の敵対象技で、予約していた相手がすでに倒れていた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ENEMY &&
              (currentTarget == nullptr || currentTarget->getHp() <= 0))
          {
            // 使用者から見た「敵」の生存者を取得
            std::vector<Status*> candidates =
                currentActor->isPlayer()
                    ? getAliveEnemies()
                    : getAliveAllies();

            if (candidates.empty()) {
              message += "\n対象がいない！";
              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            // 生存している敵からランダムで新しい対象を選ぶ
            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          // 単体蘇生の対象が、実行前に別の蘇生で既に復活していた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ALLY &&
              currentSkill.type == Skill::EffectType::REVIVE &&
              (currentTarget == nullptr || currentTarget->getHp() > 0))
          {
            message += "\n対象はすでに復活している！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          // 単体回復・単体バフの予約対象が倒れていた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ALLY &&
              (currentSkill.type == Skill::EffectType::HEAL ||
              currentSkill.type == Skill::EffectType::BUFF) &&
              (currentTarget == nullptr || currentTarget->getHp() <= 0))
          {
            // 使用者から見た生存中の味方
            std::vector<Status*> candidates =
                currentActor->isPlayer()
                    ? getAliveAllies()
                    : getAliveEnemies();

            if (candidates.empty()) {
              message =
                  String(currentActor->getname()) +
                  " の " +
                  currentSkill.name +
                  "！\n対象がいない！";

              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          // TP消費をここに集約
          if (!currentActor->useTP(currentSkill.tpCost)) {
            message = String(currentActor->getname()) + " はTPが足りない！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          message =
            String(currentActor->getname()) +
            " の " +
            currentSkill.name +
            "！";

          if (targetChanged) {
            message += "\n対象を変更した！";
          }
        }
        else {
          message = "行動指定エラー";
          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // --- 実行ロジック ---
        // A. 敵単体
        if (currentSkill.scope ==
            Skill::TargetScope::SINGLE_ENEMY)
        {
          if (currentTarget != nullptr &&
              currentTarget->getHp() > 0)
          {
            executeHostileSkillOnTarget(
                currentSkill,
                currentActor,
                currentTarget,
                music,
                true   // 挑発・ステルス・かばうあり
            );
          }
        }

        // B. 味方単体・自分
        else if (
            currentSkill.scope ==
                Skill::TargetScope::SINGLE_ALLY ||
            currentSkill.scope ==
                Skill::TargetScope::SELF)
        {
          bool targetCanReceiveSkill =
              currentTarget != nullptr &&
              (
                currentTarget->getHp() > 0 ||
                currentSkill.type ==
                    Skill::EffectType::REVIVE
              );

          if (targetCanReceiveSkill) {
            ExecuteSkill(
                currentSkill,
                currentActor,
                currentTarget,
                music
            );
          }
        }

        // C. 敵全体
        else if (
            currentSkill.scope ==
                Skill::TargetScope::ALL_ENEMIES)
        {
          bool anyDefeated = false;

          std::vector<Status*> targets =
              currentActor->isPlayer()
                  ? getAliveEnemies()
                  : getAliveAllies();

          for (auto target : targets) {

            bool defeated =
                executeHostileSkillOnTarget(
                    currentSkill,
                    currentActor,
                    target,
                    music,
                    false  // 全体攻撃なので対象変更なし
                );

            if (defeated) {
              anyDefeated = true;
            }
          }

          if (anyDefeated) {
            if (currentActor->isPlayer()) {
              message += "\n敵を倒した！";
            }
            else {
              message += "\n味方が倒れた！";
            }
          }
        }

        // D. 味方全体
        else if (
            currentSkill.scope ==
                Skill::TargetScope::ALL_ALLIES)
        {
          std::vector<Status*> targets;

          if (currentActor->isPlayer()) {

            if (currentSkill.type ==
                Skill::EffectType::REVIVE)
            {
              // 全体蘇生は死亡者も含める
              targets = getAllAllies();
            }
            else {
              targets = getAliveAllies();
            }
          }
          else {
            // 敵側
            targets = getAliveEnemies();
          }

          for (auto target : targets) {
            ExecuteSkill(
                currentSkill,
                currentActor,
                target,
                music
            );
          }
        }

        // E. ランダム敵
        else if (
            currentSkill.scope ==
                Skill::TargetScope::RANDOM_ENEMY)
        {
          int hits = 1;

          if (currentSkill.id == 123) {
            hits = random(2, 5);
          }

          if (currentSkill.id == 127) {
            hits = random(1, 8);
          }

          std::vector<Status*> targets =
              currentActor->isPlayer()
                  ? getAliveEnemies()
                  : getAliveAllies();

          if (!targets.empty()) {

            bool anyDefeated = false;

            for (int i = 0; i < hits; i++) {

              // 前のヒットで倒れている可能性があるので
              // 毎回生存者だけを作り直す
              std::vector<Status*> live;

              for (auto target : targets) {
                if (target != nullptr &&
                    target->getHp() > 0)
                {
                  live.push_back(target);
                }
              }

              if (live.empty()) {
                break;
              }

              Status* target =
                  live[random(0, live.size())];

              bool defeated =
                  executeHostileSkillOnTarget(
                      currentSkill,
                      currentActor,
                      target,
                      music,
                      true   // 1ヒットごとに対象変更あり
                  );

              if (defeated) {
                anyDefeated = true;
              }
            }

            if (anyDefeated) {
              if (currentActor->isPlayer()) {
                message += "\n敵を倒した！";
              }
              else {
                message += "\n味方が倒れた！";
              }
            }
          }
        }

        // 特殊効果メッセージ
        if (lastActionMessage != "") message += "\n" + lastActionMessage;

        // 行動後フラグ処理
        if (currentSkill.id == 86) currentActor->limitBreak();
        if (currentSkill.id == 126) {
          currentActor->cantMoveTurns = 1;
        }
        if (currentSkill.id == 85) currentActor->isDoubleAction = true; 

        // 振動処理
        if (currentTarget && !currentTarget->popupValues.empty()) {
            auto& lastPopup = currentTarget->popupValues.back();
            if (currentTarget->isPlayer() && lastPopup.value > 0) vibration.trigger(50);
            else if (!currentTarget->isPlayer() && lastPopup.isCritical) vibration.trigger(150);
            else if (!currentTarget->isPlayer() && lastPopup.value > 0) vibration.trigger(30);
        }

        currentBattleState = STATE_ACTION_MSG;
        break;
      }
      
      // 11. 行動結果のメッセージ表示 (待機のみ)
      case STATE_ACTION_MSG:
        if (aPressed) {
          hero.status->clearPopups();

          for (auto m : party.members) {
            m->status->clearPopups();
          }

          for (auto e : activeEnemies) {
            e->status->clearPopups();
          }

          if (messageFlow == MessageFlow::CONTINUE_TURN) {
            if (battlePhase == BattlePhase::EXECUTING) {
              // 毒やリジェネのメッセージを見終えたので、
              // 予約済みの行動をそのまま実行する
              currentBattleState = STATE_ACTION_CALC;
            }
            else {
              // まだ行動決定段階
              if (currentActor->isPlayer()) {
                currentBattleState = STATE_COMMAND_SELECT;
              } else {
                currentBattleState = STATE_ENEMY_AI;
              }
            }
          }
          else {
            // ACTION_COMPLETE / END_TURN のどちらでも
            // まず戦闘が終わったか確認する。
            currentBattleState = STATE_CHECK_BATTLE_RESULT;
          }
        }
        break;
      // 12. 勝利判定 (旧 8)
      case STATE_CHECK_BATTLE_RESULT: {
        // -------------------------
        // 1. 敵側が全滅しているか
        // -------------------------
        bool allEnemiesDefeated = true;

        for (auto enemy : activeEnemies) {
          if (enemy && enemy->status && enemy->status->getHp() > 0) {
            allEnemiesDefeated = false;
            break;
          }
        }

        // -------------------------
        // 2. プレイヤー側が全滅しているか
        // -------------------------
        bool anyoneAlive = false;

        if (hero.status->getHp() > 0) {
          anyoneAlive = true;
        }

        if (!anyoneAlive) {
          for (auto m : party.members) {
            if (m && m->status && m->status->getHp() > 0) {
              anyoneAlive = true;
              break;
            }
          }
        }

        // -------------------------
        // 3. 勝敗判定
        // -------------------------

        // 両陣営が同時に全滅した場合。
        // 今回は既存挙動をなるべく維持する。
        if (allEnemiesDefeated && !anyoneAlive) {
          if (currentActor && currentActor->isPlayer()) {
            message = "敵を倒した！";
            battleEndReason = BattleEndReason::VICTORY;

            if (aPressed) {
              currentBattleState = STATE_SHOW_XP_GAIN;
            }
          }
          else {
            message = "全滅した…";
            battleEndReason = BattleEndReason::DEFEAT;

            if (aPressed) {
              currentBattleState = STATE_BATTLE_END;
            }
          }

          break;
        }

        // 敵全滅
        if (allEnemiesDefeated) {
          message = "敵を倒した！";
          battleEndReason = BattleEndReason::VICTORY;

          if (aPressed) {
            currentBattleState = STATE_SHOW_XP_GAIN;
          }

          break;
        }

        // 味方全滅
        if (!anyoneAlive) {
          message = "全滅した…";
          battleEndReason = BattleEndReason::DEFEAT;

          if (aPressed) {
            currentBattleState = STATE_BATTLE_END;
          }

          break;
        }

        // -------------------------
        // 4. まだ戦闘継続
        // -------------------------
        currentBattleState = STATE_END_ACTOR_TURN;
        break;
      }
      // 13. 
      case STATE_END_ACTOR_TURN: {
        // -------------------------
        // 1. 2回行動
        // -------------------------
        // 実際に行動を完了していて、
        // かつ本人がまだ生きている場合のみ再行動する
        if (messageFlow == MessageFlow::ACTION_COMPLETE &&
            currentActor != nullptr &&
            currentActor->getHp() > 0 &&
            currentActor->isDoubleAction)
        {
          currentActor->isDoubleAction = false;

          // 前の行動情報をクリア
          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;
          messageFlow = MessageFlow::ACTION_COMPLETE;

          if (currentActor->isPlayer()) {
            message = String(currentActor->getname()) + " の再行動！";
            currentBattleState = STATE_COMMAND_SELECT;
          }
          else {
            currentBattleState = STATE_ENEMY_AI;
          }

          break;
        }

        // -------------------------
        // 2. この行動者のターン終了
        // -------------------------
        if (currentActor != nullptr) {
          currentActor->UpdateTurn();
        }

        // 次の行動に古い情報を持ち越さない
        pendingActionType = ActionType::NONE;
        pendingSkillIndex = -1;
        currentTarget = nullptr;
        messageFlow = MessageFlow::ACTION_COMPLETE;

        // -------------------------
        // 3. 次の行動者へ
        // -------------------------
        if (battlePhase == BattlePhase::EXECUTING) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
        }
        else {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
        }
        break;
      }

      // 14. XP獲得 (旧 10)
      case STATE_SHOW_XP_GAIN:
        {
          // メッセージキューをクリア
          resultMessages.clear();
          resultMessageIndex = 0;

          lastGainedXP = 0;
          for (auto enemy : activeEnemies) {
            lastGainedXP += enemy->xpYield;
          }
          
          // 1. 獲得メッセージ
          resultMessages.push_back(String(lastGainedXP) + "Expを得た!");
          
          // 2. レベルアップ・進化判定 (主人公)
          if (hero.status->getHp() > 0) {
            int res = hero.status->gainXP(lastGainedXP);
            if (res >= 1) resultMessages.push_back("調律師はLv" + String(hero.status->getLevel()) + "になった!");
          }
          // 3. レベルアップ・進化判定 (仲間)
          for(auto ally : party.members) {
            if (ally->status->getHp() > 0) {
              int allyRes = ally->status->gainXP(lastGainedXP);
              if (allyRes >= 1) resultMessages.push_back(String(ally->status->getname()) + "はLv" + String(ally->status->getLevel()) + "になった!");
              if (allyRes == 2) {
                resultMessages.push_back(
                    String(ally->status->getname()) +
                    "は進化した!"
                );
              }
            }
          }
          
          // 4. ドロップ判定
          // 敵ごとに設定されている dropItemId / dropRatePercent を使う。
          // 逃走した敵は dropItemId = 0 にされているため対象外。
          for (auto enemy : activeEnemies) {
            if (enemy == nullptr) {
              continue;
            }

            if (enemy->dropItemId <= 0 ||
                enemy->dropRatePercent <= 0)
            {
              continue;
            }

            // 0～99 の乱数がドロップ率未満なら入手
            if (random(0, 100) < enemy->dropRatePercent) {
              Item droppedItem =
                  createItemById(enemy->dropItemId);

              inventory.addItem(droppedItem);

              resultMessages.push_back(
                  String(droppedItem.name) +
                  "を手に入れた!"
              );
            }
          }

          // ★ 最初のメッセージを表示して、順次表示ステートへ
          message = resultMessages[0];
          currentBattleState = STATE_SHOW_RESULT_MESSAGE;
        }
        break;
      
      //15. 
      case STATE_SHOW_RESULT_MESSAGE:
        if (aPressed) {
          resultMessageIndex++;
          if (resultMessageIndex < resultMessages.size()) {
            message = resultMessages[resultMessageIndex];
          } else {
            // 全て表示し終わったら終了
            currentBattleState = STATE_BATTLE_END;
          }
        }
        break;
      // 16. 戦闘終了 (旧 13)
      case STATE_BATTLE_END:
        if(bPressed || aPressed){
          bool actualBossDefeated =
            isBossBattleMode &&
            battleEndReason == BattleEndReason::VICTORY;
          inBattle = false;
          if (battleEndReason == BattleEndReason::DEFEAT) {
            state = STATE_GAME_OVER;
          }
          else{
            state = STATE_GAME;
            if (!actualBossDefeated) {
              music.switchTrack(1, sd);
            }
          }
          
          // ★★★ 追加: 戦闘終了時にステータスをリセットする ★★★
          hero.status->resetBattleStats();
          std::vector<Status::PopupData>().swap(hero.status->popupValues);
          for(auto m : party.members) {
            m->status->resetBattleStats();
            std::vector<Status::PopupData>().swap(m->status->popupValues);
          }
          for (auto enemy : activeEnemies) {
              delete enemy; // Enemyとその中のStatusを削除
          }
          activeEnemies.clear(); // リストを空にする
          // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
          // ★ ベクタの容量自体を解放するテクニック (Swap idiom)
          std::vector<Enemy*>().swap(activeEnemies);

          // 3. メッセージリストのメモリ解放 (ここが2KBの犯人の可能性大)
          resultMessages.clear();
          std::vector<String>().swap(resultMessages);

          // 4. 行動順リストの解放
          actorList.clear();
          std::vector<Status*>().swap(actorList);
          if (actualBossDefeated) {
            // 撃破フラグを立てる
            int bossIndex = (map.currentFloor / 10) - 1;
            if (bossIndex >= 0 && bossIndex < 5) {
              bossFlags[bossIndex] = true;
            }

            // マップを集落に作り変える
            map.isTown = true; 
            map.regenerate(20, 20, Map::TYPE_TOWN);
            
            // プレイヤー位置再配置
            hero.reset(map); 
            
            // 画面全体を再描画 (Graphicクラス使用)
            int cx = hero.X - Graphic::SCREEN_WIDTH/2;
            int cy = hero.Y - Graphic::SCREEN_HEIGHT/2;
            g.invalidateAndRedraw(map, hero, cx, cy);

            // BGMを集落用に変更 (ID:0)
            music.switchTrack(0, sd); 
          }else if (battleEndReason == BattleEndReason::DEFEAT) {
            music.switchTrack(99, sd);
          }else {
            // 通常戦闘終了後: BGMを戻す
            if (map.isTown) music.switchTrack(0, sd); // 集落
            else music.switchTrack(1, sd);            // ダンジョン
          }
        }
        break;
      
    } // switch 終了
  }// update 終了
  
  // ★ 引数を Graphic* に変更
  void draw(Graphic* g) { 
    if (!inBattle) return;

    // ★ ここでスプライトを作るのをやめる！
    // 代わりに g->lineSprite を使う

    Caractor& hero = *heroRef;
    Party& party = *partyRef;

    // --- 分割描画ループ ---
    // Graphic::BLOCK_HEIGHT (32) を使う
    for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {
      
      g->lineSprite.fillScreen(TFT_BLACK); // sp. -> g->lineSprite.

      // 1. Yボタン（ステータス確認）の場合
      if (currentBattleState == STATE_CHECK_STATUS) {
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setTextSize(2);

        Status* s = nullptr;
        if (statusPage == 0) s = hero.status;
        else if (statusPage - 1 < party.members.size()) s = party.members[statusPage - 1]->status;
        
        if (s != nullptr) {
          // 名前とレベル
          g->lineSprite.setCursor(10, 10 - drawY);
          g->lineSprite.printf("%s Lv%d", s->getname(), s->getLevel());
          
          // HP / TP
          g->lineSprite.setCursor(10, 40 - drawY);
          g->lineSprite.printf("HP: %d / %d", s->getHp(), s->getMaxHp());
          g->lineSprite.setCursor(10, 65 - drawY);
          g->lineSprite.printf("TP: %d / %d", s->getTp(), s->getMaxTp());

          // 能力値
          g->lineSprite.setTextSize(1.5); 
          int col1 = 10; int col2 = 120;
          int startY = 100; int gap = 25;

          g->lineSprite.setCursor(col1, startY - drawY);
          g->lineSprite.printf("ATK: %d", s->getAttack());
          g->lineSprite.setCursor(col2, startY - drawY);
          g->lineSprite.printf("DEF: %d", s->getDefense());

          g->lineSprite.setCursor(col1, startY + gap - drawY);
          g->lineSprite.printf("SPD: %d", s->getSpeed());
          g->lineSprite.setCursor(col2, startY + gap - drawY);
          g->lineSprite.printf("SEN: %d", s->getSense());

          g->lineSprite.setCursor(col1, startY + gap*2 - drawY);
          g->lineSprite.printf("LCK: %d", s->getLuck());
          
          g->lineSprite.setCursor(col2, startY + gap*2 - drawY);
          g->lineSprite.printf("EXP: %d", s->getXP()); 

          // ページガイド
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_YELLOW);
          g->lineSprite.setCursor(140, 210 - drawY);
          g->lineSprite.print("<   >"); 
        }
      }
      
      // 2. 通常戦闘画面の場合
      else {
        // 背景
        g->lineSprite.pushImage(0, 0 - drawY, Graphic::SCREEN_WIDTH, Graphic::SCREEN_HEIGHT, (uint16_t*)battle_back);

        // --- 敵パーティー描画 ---
        const int enemyX[] = {10, 60, 10, 60}; 
        const int enemyY[] = {20, 40, 90, 110}; // 少しジグザグ配置
        for (int i = 0; i < activeEnemies.size(); ++i) {
          if (i >= 4) break; 
          Enemy* enemy = activeEnemies[i];
          
          if (enemy->status->getHp() > 0 || !enemy->status->popupValues.empty()) {
            int ex = enemyX[i];
            int ey = enemyY[i] - drawY; 

            if (enemy->status->getHp() > 0) {
              //uint16_t* img = (i < enemyImages.size()) ? enemyImages[i] : nullptr;
              uint16_t* img = activeImages[i];
              g->lineSprite.fillEllipse(ex + 32, ey + 60, 24, 8, TFT_RED);
              if (img) g->lineSprite.pushImage(ex, ey, 64, 64, img, H_TRANSPARENT);
              else g->lineSprite.fillRect(ex, ey, 64, 64, TFT_RED);
            }
            
            // カーソル (敵)
            if (currentBattleState == STATE_TARGET_SELECT_ENEMY && enemyTargetIndex == i && enemy->status->getHp() > 0) {
              g->lineSprite.setTextColor(TFT_YELLOW); g->lineSprite.setTextSize(2);
              g->lineSprite.setCursor(ex + 26, ey - 20); g->lineSprite.print("▼");
            }
            // ポップアップ (敵)
            if (!enemy->status->popupValues.empty()) {
              int pY = ey - 20; // 少し上から
              g->lineSprite.setTextSize(2);
              for (const auto& data : enemy->status->popupValues) {
                int val = data.value;
                bool isCrit = data.isCritical; // ★ 会心チェック

                if (val == 0) { 
                  g->lineSprite.setTextColor(TFT_LIGHTGREY);
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print("Miss");
                } else if (val > 0) { // ダメージ
                  if (isCrit) {
                     g->lineSprite.setTextColor(TFT_RED); // 会心は赤
                  } else {
                     g->lineSprite.setTextColor(TFT_YELLOW); // 通常は黄色
                  }
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print(val);
                } else { // 回復
                  g->lineSprite.setTextColor(TFT_GREEN);
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print(-val);
                }
                pY -= 20; // 間隔広め
              }
              g->lineSprite.setTextSize(1.5); // 戻す
            }
          }
        }
        
        // --- 味方パーティー描画 ---
        const int heroX = 240;
        const int heroY = 20 - drawY; // 右上
        
        // 主人公
        if (hero.status->getHp() > 0 || !hero.status->popupValues.empty()) {
          if (hero.status->getHp() > 0) {
            //uint16_t* img = (!allyImages.empty()) ? allyImages[0] : nullptr;
            uint16_t* img = activeImages[4];
            g->lineSprite.fillEllipse(heroX + 32, heroY + 60, 24, 8, TFT_CYAN);
            if (img) g->lineSprite.pushImage(heroX, heroY, 64, 64, img, H_TRANSPARENT);
            else g->lineSprite.fillRect(heroX, heroY, 64, 64, TFT_BLUE);
          }
          // ポップアップ
          if (!hero.status->popupValues.empty()) {
            int pY = heroY - 20;
            g->lineSprite.setTextSize(2);
            for (const auto& data : hero.status->popupValues) {
              int val = data.value;
              bool isCrit = data.isCritical;
              if (val == 0) { 
                g->lineSprite.setTextColor(TFT_LIGHTGREY); 
                g->lineSprite.setCursor(heroX + 10, pY); 
                g->lineSprite.print("Miss"); 
              }else if (val > 0) {
                // ★★★ 修正: 会心なら赤、通常なら黄色 ★★★
                if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                else g->lineSprite.setTextColor(TFT_YELLOW);
                // ★★★★★★★★★★★★★★★★★★★★★★★
                g->lineSprite.setCursor(heroX + 10, pY); 
                g->lineSprite.print(val);
              }
              else {
                g->lineSprite.setTextColor(TFT_GREEN); 
                g->lineSprite.setCursor(heroX + 10, pY); 
                g->lineSprite.print(-val);
              }
              pY -= 20;
            }
            g->lineSprite.setTextSize(1.5);
          }
        }

        // 仲間
        const int allyX[] = {190, 240, 190}; 
        const int allyY_base[] = {60, 90, 120}; // 右下
        for (int i = 0; i < party.members.size(); i++) {
          Automaton* ally = party.members[i];
          int ay = allyY_base[i] - drawY;

          if (ally->status->getHp() > 0 || !ally->status->popupValues.empty()) {
            if (ally->status->getHp() > 0) {
              //uint16_t* img = (i + 1 < allyImages.size()) ? allyImages[i + 1] : nullptr;
              uint16_t* img = activeImages[5 + i];
              g->lineSprite.fillEllipse(allyX[i] + 32, ay + 60, 24, 8, TFT_CYAN);
              if (img) g->lineSprite.pushImage(allyX[i], ay, 64, 64, img, H_TRANSPARENT);
              else g->lineSprite.fillRect(allyX[i], ay, 64, 64, TFT_GREEN);
            }
            // ポップアップ
            if (!ally->status->popupValues.empty()) {
              int pY = ay - 20;
              g->lineSprite.setTextSize(2);
              for (const auto& data : ally->status->popupValues) {
                int val = data.value;
                bool isCrit = data.isCritical;
                if (val == 0) {
                  g->lineSprite.setTextColor(TFT_LIGHTGREY);
                  g->lineSprite.setCursor(allyX[i] + 20, pY);
                  g->lineSprite.print("Miss");
                }else if (val > 0) {
                  // ★★★ 修正: 会心なら赤、通常なら黄色 ★★★
                  if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                  else g->lineSprite.setTextColor(TFT_YELLOW);
                  // ★★★★★★★★★★★★★★★★★★★★★★★
                  g->lineSprite.setCursor(allyX[i] + 20, pY); 
                  g->lineSprite.print(val);
                }
                else {
                  g->lineSprite.setTextColor(TFT_GREEN);
                  g->lineSprite.setCursor(allyX[i] + 20, pY);
                  g->lineSprite.print(-val);
                }
                pY -= 20;
              }
              g->lineSprite.setTextSize(1.5);
            }
          }
        }
          
        // ターゲットカーソル(味方)
        if (currentBattleState == STATE_TARGET_SELECT_ALLY) {
          bool isReviveSkill = false;

          if (currentActor &&
              pendingActionType == ActionType::SKILL) {

            std::vector<Skill>& skills =
                currentActor->getLearnedSkills();

            if (pendingSkillIndex >= 0 &&
                pendingSkillIndex < static_cast<int>(skills.size())) {
              isReviveSkill =
                  (skills[pendingSkillIndex].type ==
                  Skill::EffectType::REVIVE);
            }
          }

          std::vector<Status*> allies =
              isReviveSkill ? getDeadAllies()
                            : getAliveAllies();

          if (allyTargetIndex < allies.size()) {
            Status* targetedAlly = allies[allyTargetIndex];
            int tx = 0, ty = 0;
            if (targetedAlly == hero.status) { tx = 220; ty = 20; } 
            else {
              for (int i = 0; i < party.members.size(); i++) {
                if (party.members[i]->status == targetedAlly) {
                  tx = allyX[i]; ty = allyY_base[i];
                  break;
                }
              }
            }
            if (tx != 0) {
              g->lineSprite.setTextColor(TFT_YELLOW); 
              g->lineSprite.setTextSize(2);
              g->lineSprite.setCursor(tx + 26, ty - 20 - drawY); // 位置調整
              g->lineSprite.print("▼");
            }
          }
        }

        // --- コマンドウィンドウ (小型化・中央下) ---
        int cmdX = 80; 
        int cmdY = 170 - drawY; 
        int cmdW = 160; 
        int cmdH = 65; 

        g->lineSprite.fillRect(cmdX, cmdY, cmdW, cmdH, TFT_DARKGREY);
        g->lineSprite.drawRect(cmdX, cmdY, cmdW, cmdH, TFT_WHITE);
        g->lineSprite.setTextSize(1);
        
        if (currentBattleState == STATE_COMMAND_SELECT) {
          int x_offset = cmdX + 10;
          int y_offset = cmdY + 10;
          for (int i = 0; i < CMD_COUNT; i++) {
            uint16_t color = (i == commandSelected) ? TFT_YELLOW : TFT_WHITE;
            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x_offset, y_offset);
            g->lineSprite.print(commands[i]);
            if(i % 2 == 0) x_offset += 70;
            if(i % 2 == 1){ y_offset += 25; x_offset -= 70; }
          }
        } 
        else if (currentBattleState == STATE_SKILL_SELECT) {
          if (currentActor) {
            std::vector<Skill>& skills = currentActor->getLearnedSkills();
            int skillCount = skills.size();
            // スクロール計算
            int itemsPerPage = 3;
            int startIdx = 0;
            if (skillSelected >= itemsPerPage) {
              startIdx = skillSelected - (itemsPerPage - 1);
            }
            int endIdx = startIdx + itemsPerPage;
            if (endIdx > skillCount) endIdx = skillCount;

            for (int i = startIdx; i < endIdx; i++) {
              uint16_t color = (i == skillSelected) ? TFT_YELLOW : TFT_WHITE;
              g->lineSprite.setTextColor(color);
              int relativeIndex = i - startIdx;
              g->lineSprite.setCursor(cmdX + 10, cmdY + 5 + (relativeIndex * 20)); 
              g->lineSprite.printf("%s(%d)", skills[i].name, skills[i].tpCost);
            }
          }
        }

        // --- メッセージ (最下部に移動) ---
        // ★★★ 修正: 座標を 110 -> 220 に変更して、キャラと被らないようにする ★★★
        g->lineSprite.setTextSize(1);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(10, 220 - drawY); // ここ！
        g->lineSprite.print(message);
      }

      // 画面に転送
      g->lineSprite.pushSprite(0, drawY);
    }
  }
  ~Battle() {
    // 残っている敵がいれば削除
    for (auto enemy : activeEnemies) {
        delete enemy;
    }
    activeEnemies.clear(); 
    // (activeImages は ImageManager が管理しているのでここでは何もしなくてOK)
  }
private:
  
  void ExecuteSkill(Skill& skill, Status* user, Status* target, MUSIC& music) {
    if (target == nullptr) return;

    int value = 0; 
    bool isCrit = false;
    switch (skill.type) {
      
      case Skill::EffectType::DAMAGE:
        // ★★★ ここから修正 ★★★
        // (user->getSense() と target->getSense() でバフ/デバフが反映された値を取得)
        value = user->caclulateSkillDamage(target, skill, isCrit);
        if (isCrit) music.playSE(4); // 会心
        else {
          if (skill.id == 0) music.playSE(3); // 通常攻撃
          else if (skill.category == Skill::Category::PHYSICAL) {
            if (skill.scope == Skill::TargetScope::ALL_ENEMIES) music.playSE(6); // 物理全体
            else music.playSE(5); // 物理単体
          } else {
            if (skill.scope == Skill::TargetScope::ALL_ENEMIES) music.playSE(8); // センス全体
            else music.playSE(7); // センス単体
          }
        }
        target->takedamage(value, skill.category);
        target->addPopup(value, isCrit); // ★ 結果をポップアップへ
        lastDamage += value; 
        break;

      case Skill::EffectType::HEAL:
        // ★ isCrit を渡す
        music.playSE(7);
        value = user->calculateHealAmount(skill, isCrit);
        target->ReceiveHeal(value);
        target->addPopup(-value, isCrit); // ★ 結果をポップアップへ (回復も赤字/強調表示される)
        lastDamage -= value; 
        break;

      case Skill::EffectType::MULTI_HIT_DAMAGE:
        {
          int hits = 1;
          if (skill.id == 23 || skill.id == 74 || skill.id == 76) hits = 2; 
          if (skill.id == 36) hits = 3;

          for(int i=0; i<hits; i++) {
            isCrit = false; // 毎回リセット
            // ★ isCrit を渡す
            value = user->caclulateSkillDamage(target, skill, isCrit);
            if (isCrit) music.playSE(4);
            else {
              // 物理/センス判定
              if (skill.category == Skill::Category::PHYSICAL) music.playSE(5); 
              else music.playSE(7);
            }
            target->takedamage(value, skill.category);
            target->addPopup(value, isCrit); // ★ 結果をポップアップへ
            lastDamage += value; 
          }
        }
        break;

      case Skill::EffectType::BUFF:
        target->AddBuff(skill.id);
        music.playSE(7);
        lastActionMessage = "能力UP!"; // 短く
        break;
      case Skill::EffectType::DEBUFF:
        if(target->AddDebuff(skill.id, user)){
          music.playSE(7);
          lastActionMessage = "能力DOWN!";
        }else {
          // ★ 失敗時
          lastActionMessage = "レジスト！"; // または "かからなかった！"
        }
        break;
      case Skill::EffectType::CURE_STATUS:
        target->CureStatus(skill.id == 91);
        music.playSE(7); 
        lastActionMessage = "治った！";
        break;
      case Skill::EffectType::REVIVE:
        if (skill.id == 98) target->Revive(100); 
        else target->Revive(50);
        music.playSE(7);
        lastActionMessage = "復活！";
        break;
      
      case Skill::EffectType::SPECIAL:
        if (skill.id == 100) { 
          music.playSE(7);
          target->ReceiveTP(skill.power); 
          lastActionMessage = "TP回復";
        } // 2. アサシネイト (即死)
        else if (skill.id == 42) {
          int chance = 30 + (user->getLuck() - target->getLuck()); // 基本30%
          if (random(0, 100) < chance) {
            music.playSE(4);
            int dmg = target->getHp();
            target->takedamage(dmg, skill.category);
            target->addPopup(dmg, true); // 赤字
            lastActionMessage = "急所を突いた! 即死!";
          } else {
            // 失敗時は通常ダメージの半分
            music.playSE(5);
            value = user->caclulateSkillDamage(target, skill, isCrit) / 2;
            if(value<1) value=1;
            target->takedamage(value, skill.category);
            target->addPopup(value, isCrit);
            lastActionMessage = "急所を外した...";
          }
        }

        // 3. ペインサイクル / TPドレイン (吸収)
        else if (skill.id == 44 || skill.id == 111) {
          if (skill.id == 44) { // HP吸収
            music.playSE(5);
            value = user->caclulateSkillDamage(target, skill, isCrit);
            target->takedamage(value, skill.category);
            target->addPopup(value, isCrit);
            user->ReceiveHeal(value); // 自分回復
            user->addPopup(-value, false);
            lastActionMessage = "体力を奪い取った!";
          } else { // TP吸収
            music.playSE(7);
            int drain = 30;
            target->ReceiveTP(-drain);
            user->ReceiveTP(drain);
            lastActionMessage = "TPを奪い取った!";
          }
        }

        // 4. 攻撃 + 状態異常 (ポイズン、ヴォイドショック、ディザスターなど)
        // (ID: 105, 45, 110, 114)
        else if (skill.id == 105 ||
          skill.id == 45 ||
          skill.id == 110 ||
          skill.id == 114 ||
          skill.id == 122)
        {
          // まず攻撃の命中・ダメージ判定
          value =
              user->caclulateSkillDamage(
                  target,
                  skill,
                  isCrit
              );

          target->takedamage(value, skill.category);
          target->addPopup(value, isCrit);

          // ダメージ計算結果が0なら攻撃自体が失敗したので、
          // 追加の状態異常も発生させない
          if (value <= 0) {
            lastActionMessage = "ミス！";
          }
          else {
            bool success =
                target->AddDebuff(skill.id, user);

            if (skill.id == 105 && success) {
              lastActionMessage = "毒を与えた!";
              music.playSE(7);
            }
            else if (skill.id == 110 && success) {
              lastActionMessage = "猛毒を与えた!";
              music.playSE(7);
            }
            else if (skill.id == 45 && success) {
              lastActionMessage = "麻痺を与えた!";
              music.playSE(5);
            }
            else if (skill.id == 114 && success) {
              lastActionMessage = "厄災を与えた!";
              music.playSE(7);
            }
            else if (skill.id == 122 && success) {
              lastActionMessage = "敵をかく乱した!";
              music.playSE(7);
            }
          }
        }

        // 5. かばう / カウンター / 挑発 (自分にバフ)
        else if (skill.id == 27 || skill.id == 53 || skill.id == 49) {
          target->AddBuff(skill.id);
          music.playSE(5);
          if (skill.id == 27) lastActionMessage = "身代わりの構え!";
          else if (skill.id == 53) lastActionMessage = "反撃の構え!";
          else lastActionMessage = "挑発した!";
        }
        
        // 6. タービュランス (全体風属性+SPDダウン)
        else if (skill.id == 87) {
          music.playSE(6);

          value =
              user->caclulateSkillDamage(
                  target,
                  skill,
                  isCrit
              );

          target->takedamage(value, skill.category);
          target->addPopup(value, isCrit);

          if (value <= 0) {
            lastActionMessage = "ミス！";
          }
          else {
            target->AddDebuff(skill.id, user);
            lastActionMessage = "風が切り刻む!";
          }
        }

        else if (skill.id == 32) {
          // Status::AddBuff(32) で reflectAllTurns = 1 がセットされます
          target->AddBuff(skill.id); 
          music.playSE(7); // バフ音
          lastActionMessage = "物理と旋律を跳ね返す！";
        }

        else if (skill.id == 24) {
          music.playSE(5);

          value =
              user->caclulateSkillDamage(
                  target,
                  skill,
                  isCrit
              );

          target->takedamage(value, skill.category);
          target->addPopup(value, isCrit);

          if (value <= 0) {
            lastActionMessage = "ミス！";
          }
          else {
            if (target->AddDebuff(skill.id, user)) {
              lastActionMessage = "装甲を破壊した！";
            }
          }
        }

        else if (skill.id == 47) {
          music.playSE(5);

          value =
              user->caclulateSkillDamage(
                  target,
                  skill,
                  isCrit
              );

          target->takedamage(value, skill.category);
          target->addPopup(value, isCrit);

          if (value <= 0) {
            lastActionMessage = "ミス！";
          }
          else {
            if (target->AddDebuff(skill.id, user)) {
              lastActionMessage = "気絶させた！";
            }
          }
        }

        // その他 (未実装)
        else {
          lastActionMessage = "しかし何も起こらなかった。";
        }
        break;

      default:
        lastActionMessage = "ミス！";
        target->addPopup(0, false); // ★ 0はミス扱い
        break;
    }
  }

  // 生きている味方のリストを取得する
  std::vector<Status*> getAliveAllies() {
    std::vector<Status*> allies;
    // 主人公
    if (heroRef->status->getHp() > 0) {
      allies.push_back(heroRef->status);
    }
    // 仲間
    for (auto ally : partyRef->members) {
      if (ally->status->getHp() > 0) {
        allies.push_back(ally->status);
      }
    }
    return allies;
  }

  std::vector<Status*> getDeadAllies() {
    std::vector<Status*> allies;

    // 主人公
    if (heroRef && heroRef->status &&
        heroRef->status->getHp() <= 0) {
      allies.push_back(heroRef->status);
    }

    // 仲間
    if (partyRef) {
      for (auto ally : partyRef->members) {
        if (ally && ally->status &&
            ally->status->getHp() <= 0) {
          allies.push_back(ally->status);
        }
      }
    }

    return allies;
  }

  std::vector<Status*> getAllAllies() {
    std::vector<Status*> allies;

    if (heroRef && heroRef->status) {
      allies.push_back(heroRef->status);
    }

    if (partyRef) {
      for (auto ally : partyRef->members) {
        if (ally && ally->status) {
          allies.push_back(ally->status);
        }
      }
    }

    return allies;
  }

  std::vector<Status*> getAliveEnemies() {
    std::vector<Status*> enemies;

    for (auto enemy : activeEnemies) {
      if (enemy && enemy->status && enemy->status->getHp() > 0) {
        enemies.push_back(enemy->status);
      }
    }

    return enemies;
  }
};

struct GameContext {
  Map* map;
  Caractor* hero;
  Graphic* graphic;
  Controller* controller;
  MUSIC* music;
  Sd* sd;
  Menu* menu;
  Encounter* encounter;
  Inventory* inventory;
  Party* party;
  GameState state;
  EventType currentEvent = EVENT_NONE;
  SaveManager* saveManager;
  Battle* battle;
  bool requestRegenerate;
  Vibration* vibration;
  ImageManager* imgMgr;
  MemoryMonitor memMonitor;
  Config config;
  bool bossDefeated[5] = {false, false, false, false, false};
};

GameContext* ctx = nullptr;

// タイマー割り込み関数

bool audio_timer_callback(struct repeating_timer *t) {
  MUSIC* music = ctx->music;
  
  // ★ 修正: BGMが一時停止中でもSEだけは鳴るようにする
  // if (music->paused) return true; // ← これを削除またはSE用に変更

  int32_t mixedSample = 0;

  // --- 1. BGMの取得 ---
  if (!music->paused && music->sampleIndex + 1 < music->playBytes) {
    uint8_t lo = music->playBuf[music->sampleIndex];
    uint8_t hi = music->playBuf[music->sampleIndex + 1];
    int16_t bgm = (int16_t)(lo | (hi << 8));
    bgm = (int16_t)(bgm * music->bgmVolume);
    mixedSample += bgm;
    music->sampleIndex += 2;
  } else {
    // バッファ切れまたは停止中はBGM音量0
    // refill要求などはここで行う
    if (!music->paused &&
        music->fillBytes > 0 &&
        music->sampleIndex >= music->playBytes)
    {
      noInterrupts();

      // 読み込み済みの裏バッファを再生側へ切り替える
      volatile uint8_t* tmp = music->playBuf;
      music->playBuf = music->fillBuf;
      music->fillBuf = (uint8_t*)tmp;

      music->playBytes = music->fillBytes;
      music->fillBytes = 0;
      music->sampleIndex = 0;

      // 今まで再生していたバッファが空いたので、
      // core1へ次のデータ補充をすぐ要求する
      music->needs_refill = true;

      interrupts();
    }
    else if (!music->paused &&
            music->fillBytes == 0)
    {
      // 裏バッファがまだ準備できていない場合も補充要求を出す
      music->needs_refill = true;
    }
  }

  // --- 2. SE (シンセサイザー生成) ★ここが変わった！ ---
  if (music->seType != MUSIC::OFF) {
    int16_t seWave = 0;

    // 音量の減衰
    music->seVol -= music->seVolDecay;
    if (music->seVol <= 0) {
      music->seType = MUSIC::OFF; // 音量0で終了
      music->seVol = 0;
    } else {
      // 周波数の変化 (スライド)
      music->seFreq += music->seFreqSlide;
      
      // 波形生成
      if (music->seType == MUSIC::SQUARE) {
        // 矩形波: 位相を進めて High/Low を切り替え
        // 16000Hz動作なので、位相は Freq / 16000 ずつ進む
        music->sePhase += music->seFreq / 16000.0f;
        if (music->sePhase >= 1.0f) music->sePhase -= 1.0f;
        
        seWave = (music->sePhase < 0.5f) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      } 
      else if (music->seType == MUSIC::NOISE) {
        // ノイズ: 乱数
        // (高速化のため簡易的な乱数生成)
        static uint16_t lfsr = 0xACE1;
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        seWave = (lfsr & 1) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      }
    }
    
    mixedSample += seWave;
  }

  // --- 3. クリッピング (音割れ防止) ---
  if (mixedSample > 32767) mixedSample = 32767;
  if (mixedSample < -32768) mixedSample = -32768;

  // --- 4. DACへ出力 ---
  // 16bit signed -> 12bit unsigned (0〜4095)
  uint16_t usample = (uint16_t)(mixedSample + 32768) >> 6;
  if (usample > 4095) usample = 4095;
  music->setDAC_ISR(usample);
  
  return true;
}

// -------------------------
// core1 のエントリポイント（オーディオ専用）
// -------------------------
void core1_main() {
  ctx->sd->init(*ctx->music);

  // -------------------------------------------------
  // 音声専用タイマーをcore1上に作成
  //
  // デフォルトalarm poolは使わず、
  // このcore1上で独自のalarm poolを作ることで、
  // audio_timer_callbackもcore1側のIRQとして実行する。
  // -------------------------------------------------
  static alarm_pool_t* audioAlarmPool =
      alarm_pool_create_with_unused_hardware_alarm(1);

  static repeating_timer_t audioTimer;

  // 16kHzの理想周期は62.5us。
  // repeating_timerは整数us指定なので、
  // 近い63usを使用する。
  //
  // 負値にすることで
  // 「callback終了から63us後」ではなく
  // 「callback開始から63usごと」に呼ばれる。
  bool timerStarted =
      alarm_pool_add_repeating_timer_us(
          audioAlarmPool,
          -63,
          audio_timer_callback,
          nullptr,
          &audioTimer
      );

  // タイマー生成に失敗した場合は、
  // 不定状態でゲームを続けない。
  if (!timerStarted) {
    while (true) {
      tight_loop_contents();
    }
  }

  while (true) {
    ctx->sd->performPendingSwitch(*ctx->music);
    ctx->sd->refile(*ctx->music);

    // ISRが止まっていてrefill完了したら再開
    if (ctx->music->paused &&
        !ctx->music->needs_refill)
    {
      ctx->music->paused = false;
    }

    tight_loop_contents();
  }
}

void Menu::update(Controller &ctrl, Inventory& inventory, Caractor& hero, Party& party, Map& map, MUSIC& music, Sd& sd, SaveManager& saveMgr, Config& config, Vibration& vib, bool* bossFlags) {
  if (!active) return; // メニューが非表示なら何もしない
  bool aPressed = ctrl.pressedDebounced(ctrl.BTN_A);
  bool bPressed = ctrl.pressedDebounced(ctrl.BTN_B);
  bool upPressed = ctrl.pressedDebounced(ctrl.BTN_UP);
  bool downPressed = ctrl.pressedDebounced(ctrl.BTN_DOWN);
  bool rightPressed = ctrl.pressedDebounced(ctrl.BTN_RIGHT);
  bool leftPressed = ctrl.pressedDebounced(ctrl.BTN_LEFT);
  
  // --- Bボタン（キャンセル）処理 ---
  if (bPressed) {
    music.playSE(1);
    if (currentMenuState == STATE_ITEM_TARGET) {
      currentMenuState = STATE_ITEM; // 対象選択から道具一覧へ戻る
      return;
    }else if (currentMenuState == STATE_STORAGE) {
      if (storageMode != 0) storageMode = 0; // モード選択に戻る
      else active = false; // メニューを閉じる (イベント終了)
      return;
    }else if (currentMenuState == STATE_EQUIP_LIST) {
      currentMenuState = STATE_EQUIP_SLOT; 
    }else if (currentMenuState == STATE_SHOP_BUY || currentMenuState == STATE_SHOP_SELL) {
      currentMenuState = STATE_SHOP_CHOICE; // 選択画面に戻る
    }else if (currentMenuState == STATE_SHOP_CHOICE) {
      active = false; // 店を出る
    }else if (currentMenuState == STATE_INN) {
      active = false; // 宿屋を出る
    }else if (currentMenuState == STATE_EQUIP_SLOT||currentMenuState == STATE_EVOLVE || currentMenuState == STATE_SKILL || currentMenuState == STATE_ITEM || currentMenuState == STATE_CONFIG) {
      // 進化・旋律画面でBを押したらメインに戻る
      currentMenuState = STATE_MAIN;
    } else if (currentMenuState == STATE_MAIN) {
      // メインメニューでBを押したらメニューを閉じる
      active = false;
    }
    return; // Bボタンが押されたフレームは他の操作を無視
  }

  // --- 状態別 Aボタン（決定）とカーソル移動 ---
  switch (currentMenuState) {
    
    case STATE_MAIN: {
      // --- メインメニューの操作 ---
      if (upPressed){
        mainSelected = (mainSelected - 1 + MAIN_ITEM_COUNT) % MAIN_ITEM_COUNT;
        music.playSE(2);
      }
      if (downPressed){
        mainSelected = (mainSelected + 1) % MAIN_ITEM_COUNT;
        music.playSE(2);
      }
      if (aPressed) {
        music.playSE(0);
        if (mainSelected == 0) {
          currentMenuState = STATE_ITEM;
          listCursor = 0;
        }else if (mainSelected == 1) { // 「旋律」が選ばれた
          currentMenuState = STATE_SKILL;
          listCursor = 0; 
        }
        else if (mainSelected == 2) { // 「進化」が選ばれた
          currentMenuState = STATE_EVOLVE;
          listCursor = 0;
        }else if (mainSelected == 3) { 
          currentMenuState = STATE_EQUIP_SLOT; 
          listCursor = 0; 
        }else if (mainSelected == 4) { // 「記録」
          saveSucceeded =
              saveMgr.saveGame(sd, music, map, hero, party, inventory, bossFlags);

          currentMenuState = STATE_SAVE_RESULT;
        }else if (mainSelected == 5) {
          currentMenuState = STATE_CONFIG;
          listCursor = 0;
        }
      }
      break;
    }
    case STATE_ITEM: {
      // ★ 修正：全アイテムを対象にする (フィルタリングしない)
      int itemCount = inventory.items.size();

      if (itemCount > 0) {
        if (upPressed){
          listCursor = (listCursor - 1 + itemCount) % itemCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % itemCount;
          music.playSE(2);
        }
        if (aPressed) {
          // カーソル位置のアイテムIDを確認
          int id = inventory.items[listCursor].item.id;
          
          // ★ 消費アイテム (100〜199) の場合のみ使用可能
          if (id >= 100 && id < 200) {
            music.playSE(0);
            selectedItemIndex = listCursor; // そのままインデックスとして保存
            currentMenuState = STATE_ITEM_TARGET;
            listCursor = 0; // ターゲット選択のカーソル初期化
          } else {
            music.playSE(1);
            // 使えないアイテムは何もしない
          }
        }
      }
      break;
    }
    case STATE_ITEM_TARGET: {
      // 対象リスト: [0]主人公, [1~]仲間
      int totalTargets = 1 + party.members.size(); 

      if (upPressed){
        listCursor = (listCursor - 1 + totalTargets) % totalTargets;
        music.playSE(2);
      }
      if (downPressed){
        listCursor = (listCursor + 1) % totalTargets;
        music.playSE(2);
      }

      if (aPressed) {
        music.playSE(0);
        // 対象のStatus特定
        Status* targetStatus = nullptr;
        if (listCursor == 0) targetStatus = hero.status;
        else targetStatus = party.members[listCursor - 1]->status;

        // アイテム効果適用
        int itemId = inventory.items[selectedItemIndex].item.id;
        
        if (itemId == 100) targetStatus->ReceiveHeal(50);
        else if (itemId == 101) targetStatus->ReceiveHeal(200);
        else if (itemId == 102) targetStatus->ReceiveHeal(9999);
        else if (itemId == 103) targetStatus->ReceiveTP(20);
        else if (itemId == 104) targetStatus->CureStatus(true);

        // 消費して戻る
        inventory.removeByIndex(selectedItemIndex);
        currentMenuState = STATE_ITEM;
        listCursor = 0;
      }
      break;
    }
    case STATE_STORAGE: {
      // storageMode: 0=選択, 1=引き出す(Withdraw), 2=預ける(Deposit)
      
      if (storageMode == 0) {
        // メニュー選択 (上:引き出す, 下:預ける)
        if (upPressed || downPressed) {
          listCursor = (listCursor == 0) ? 1 : 0;
          music.playSE(2);
        }
        if (aPressed) {
          storageMode = (listCursor == 0) ? 1 : 2; // 0なら引き出す、1なら預ける
          listCursor = 0;
          music.playSE(0);
        }
      }
      else if (storageMode == 1) { // 引き出す (Storage -> Party)
        int sCount = party.storage.size();
        if (sCount > 0) {
          if (upPressed){
            listCursor = (listCursor - 1 + sCount) % sCount;
            music.playSE(2);
          }
          if (downPressed){
            listCursor = (listCursor + 1) % sCount;
            music.playSE(2);
          }
          
          if (aPressed) {
            if (party.members.size() < Party::MAX_MEMBERS) {
              party.withdrawMember(listCursor);
              listCursor = 0; // リセット
              music.playSE(0);
            }else{
              music.playSE(1);
            }
          }
        }
      }
      else if (storageMode == 2) { // 預ける (Party -> Storage)
        int pCount = party.members.size();
        if (pCount > 0) {
          if (upPressed){
            listCursor = (listCursor - 1 + pCount) % pCount;
            music.playSE(2);
          }
          if (downPressed){
            listCursor = (listCursor + 1) % pCount;
            music.playSE(2);
          }
          if (aPressed) {
            party.depositMember(listCursor);
            if (listCursor >= party.members.size()) listCursor = 0;
            music.playSE(0);
          }
        }
      }
      break;
    }
    case STATE_EVOLVE: {
      // --- 進化画面（仲間選択）の操作 ---
      int memberCount = party.members.size();
      if (memberCount > 0) { // 仲間が1人以上いる
        if (upPressed){
          listCursor = (listCursor - 1 + memberCount) % memberCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % memberCount;
          music.playSE(2);
        }

        if (aPressed) {
          Automaton* selectedAlly = party.members[listCursor];
          bool itemUsed = false; 
          
          // 進化テーブルを検索
          for (int i = 0; i < Status::evolutionRuleCount; i++) {
            const Status::EvolutionRule& rule = Status::evolutionTable[i];
            
            // 1. この仲間の進化ルールか？
            // 2. トリガーはアイテムか？
            if (rule.before_who_id == selectedAlly->status->getWho() && 
                rule.trigger_type == Status::EvolutionTriggerType::TRIGGER_ITEM) 
            {
              int requiredItemId = rule.trigger_value; // 必要なアイテムID
              
              // 3. そのアイテムを持っているか？
              int foundIndex = -1;
              for(int j=0; j < inventory.items.size(); j++) {
                if (inventory.items[j].item.id == requiredItemId) {
                  foundIndex = j;
                  break;
                }
              }

              // 4. 持っていれば進化実行
              if (foundIndex != -1) {
                selectedAlly->status->evolve(rule.after_who_id);
                
                // ★ ここで消費する (この変数はここなら使える)
                inventory.removeByIndex(foundIndex);
                music.playSE(0);
                itemUsed = true;
                break; // 進化したらループ終了
              }
            }
            music.playSE(1);
          }

          // --- 進化後の後処理 ---
          if (itemUsed) {
            // ★ ここにあった erase は削除しました (上で既に消しているため)
            
            // カーソル位置を安全な場所に補正 (アイテムが減ってズレる可能性があるため)
            int newItemCount = inventory.items.size();
            // アイテムリストの方のカーソルではなく、仲間リストのカーソル(listCursor)なので
            // 実はここはあまり関係ないですが、念のためバグ防止
            if (listCursor >= memberCount) {
                listCursor = memberCount - 1;
            }
            // (もしアイテム画面ならアイテムリストのカーソル補正が必要ですが、
            //  ここは進化画面なので、仲間リストのカーソルはそのままで大丈夫です)
          }
        } // Aボタン処理 終了
      } // itemCount > 0 終了
      break;
    }
    case STATE_SKILL: {
      // --- ★ 旋律画面の操作 ★ ---
      if (learnableSkillCount > 0) {
        // カーソル移動
        if (upPressed) {
          listCursor = (listCursor - 1 + learnableSkillCount) % learnableSkillCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % learnableSkillCount;
          music.playSE(2);
        }

        // Aボタン（決定）
        if (aPressed) {
          int skillIdToLearn = learnableSkillIDs[listCursor];
          int requiredScrollId = 200 + skillIdToLearn; // 巻物のID

          // ★ 1. すでに覚えているかチェック
          bool alreadyLearned = false;
          for (const auto& s : hero.status->getLearnedSkills()) {
            if (s.id == skillIdToLearn) {
              alreadyLearned = true;
              break;
            }
          }

          if (alreadyLearned) {
            music.playSE(1);
            // すでに覚えているなら何もしない (SEを鳴らすならここ)
          } 
          // ★ 2. 持っているかチェック
          else if (inventory.countItem(requiredScrollId) > 0) {
              // 消費して習得
              inventory.removeItem(requiredScrollId);
              hero.status->learnSkill(Skill(skillIdToLearn));
              music.playSE(0);
              // (※ここで「覚えた！」というフラグを立てて、draw側でメッセージを出すと親切です)
          } else {
              // (巻物がない)
              music.playSE(1);
          }
        }
      }
      break;
    }
    case STATE_SAVE_RESULT:
      if (aPressed) { active = false; music.playSE(1);} // 閉じる
      break;
    case STATE_EQUIP_SLOT: {
      // 0:武器, 1:防具, 2:装飾
      if (upPressed)   { listCursor = (listCursor - 1 + 3) % 3; music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % 3; music.playSE(2); }
      if (aPressed) {
          music.playSE(0);
          equipSlot = listCursor; 
          currentMenuState = STATE_EQUIP_LIST;
          listCursor = 0;
      }
      break;
    }
    case STATE_EQUIP_LIST: {
      std::vector<int> equippableIndices;
      Item::ItemType targetType = Item::ITEM_WEAPON;
      if (equipSlot == 1) targetType = Item::ITEM_ARMOR;
      if (equipSlot == 2) targetType = Item::ITEM_ACCESSORY;

      for(int i=0; i<inventory.items.size(); i++) {
        if (inventory.items[i].item.type == targetType) {
          equippableIndices.push_back(i);
        }
      }
      
      int count = equippableIndices.size();
      if (count > 0) {
        if (upPressed)   { listCursor = (listCursor - 1 + count) % count; music.playSE(2); }
        if (downPressed) { listCursor = (listCursor + 1) % count; music.playSE(2); }
        
        if (aPressed) {
          music.playSE(0);
          int invIndex = equippableIndices[listCursor];
          Item newItem = inventory.items[invIndex].item; 

          // ★ 主人公固定
          Status* s = hero.status;
          
          // 1. 外す (インベントリに戻す)
          int oldId = -1;
          if (equipSlot == 0) oldId = s->equipWeaponId;
          if (equipSlot == 1) oldId = s->equipArmorId;
          if (equipSlot == 2) oldId = s->equipAccId;

          if (oldId != -1) {
            inventory.addItem(createItemById(oldId));
          }

          // 2. 装備する
          if (equipSlot == 0) s->equipWeaponId = newItem.id;
          if (equipSlot == 1) s->equipArmorId = newItem.id;
          if (equipSlot == 2) s->equipAccId = newItem.id;

          // 3. 消費
          inventory.removeByIndex(invIndex);

          currentMenuState = STATE_EQUIP_SLOT;
        }
      }
      break;
    }
    case STATE_INN: {
      // 0: はい, 1: いいえ
      if (upPressed || downPressed) { listCursor = (listCursor == 0) ? 1 : 0; music.playSE(2);}
      
      if (aPressed) {
        if (listCursor == 0) { // はい
          int cost = 10; // 宿泊費
          if (inventory.money >= cost) {
            music.playSE(0); // 決定音 (回復音でもOK)
            inventory.money -= cost;
            
            // 全回復
            hero.status->Revive(100);
            hero.status->ReceiveHeal(9999); hero.status->ReceiveTP(9999);
            for(auto m : party.members) {
              m->status->Revive(100);
              m->status->ReceiveHeal(9999); m->status->ReceiveTP(9999);
            }
            // (ここで「回復した！」メッセージを出したいが、簡易的に閉じる)
            active = false; 
          } else {
            music.playSE(1); // お金が足りない
          }
        } else { // いいえ
          active = false; music.playSE(1);
        }
      }
      if (bPressed) { active = false; music.playSE(1); }
      break;
    }
    case STATE_SHOP_CHOICE: {
      if (upPressed)   { listCursor = (listCursor - 1 + 3) % 3;music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % 3;music.playSE(2); }
      
      if (aPressed) {
        music.playSE(0);
        if (listCursor == 0) { // 「買う」
          currentMenuState = STATE_SHOP_BUY;
          listCursor = 0; 

          // ★★★ 商品リストの生成ロジック ★★★
          shopItemList.clear();
          
          // 1. 常備薬 (階層が進むと良い薬が入荷)
          shopItemList.push_back(100); // リペアキット
          shopItemList.push_back(103); // エネルギー缶
          shopItemList.push_back(104); // 万能オイル
          if (map.currentFloor >= 10) shopItemList.push_back(101); // リペアキット中

          // 2. 階層に応じた装備品 (Tier計算)
          // 1-10F: Tier 0, 11-20F: Tier 1, ...
          int tier = (map.currentFloor - 1) / 10;
          if (tier > 4) tier = 4; // 最大Tier 4 (5段階目)

          // 武器: 300 + tier
          shopItemList.push_back(300 + tier); 
          
          // 防具: 400 + tier
          shopItemList.push_back(400 + tier);

          // アクセ: たまに並ぶとか、固定とか
          if (tier >= 1) shopItemList.push_back(500); // 11F以降

        } else if (listCursor == 1) { // 売る
          currentMenuState = STATE_SHOP_SELL;
          listCursor = 0;
        } else { // 出る
          active = false;
        }
      }
      if (bPressed) { active = false; music.playSE(1); }
      break;
    }
    case STATE_SHOP_BUY: {
      int itemCount = shopItemList.size();
      
      if (upPressed)   { listCursor = (listCursor - 1 + itemCount) % itemCount; music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % itemCount; music.playSE(2); }
      
      if (aPressed) {
        int itemId = shopItemList[listCursor]; // リストからID取得
        Item targetItem = createItemById(itemId);
        
        if (inventory.money >= targetItem.price) {
          music.playSE(0); 
          inventory.money -= targetItem.price;
          inventory.addItem(targetItem);
        } else {
          music.playSE(1); 
        }
      }
      if (bPressed) {
        currentMenuState = STATE_SHOP_CHOICE;
        listCursor = 0; // カーソル位置戻す
        music.playSE(1);
      }
      break;
    }
    case STATE_SHOP_SELL: {
      int itemCount = inventory.items.size();
      if (itemCount > 0) {
        if (upPressed)   { listCursor = (listCursor - 1 + itemCount) % itemCount; music.playSE(2); }
        if (downPressed) { listCursor = (listCursor + 1) % itemCount; music.playSE(2); }
        
        if (aPressed) {
          Item& item = inventory.items[listCursor].item;
          int sellPrice = item.price / 2; // 売値は半額
          if (sellPrice < 1) sellPrice = 1;

          music.playSE(0); // チャリーン
          inventory.money += sellPrice;
          inventory.removeByIndex(listCursor);
          
          // カーソル位置調整
          if (listCursor >= inventory.items.size()) listCursor = max(0, (int)inventory.items.size() - 1);
        }
      }
      if (bPressed) {
        currentMenuState = STATE_SHOP_CHOICE;
        listCursor = 1; // 「売る」にカーソルを戻す
        music.playSE(1);
      }
      break;
    }
    case STATE_CONFIG: {
      // 項目: 0:BGM, 1:SE, 2:振動, 3:戻る
      if (upPressed)   { listCursor = (listCursor - 1 + 4) % 4; music.playSE(2);}
      if (downPressed) { listCursor = (listCursor + 1) % 4; music.playSE(2);}

      // 左右キーで値を変更
      if (leftPressed || rightPressed) {
        int change = rightPressed ? 1 : -1;
        bool valChanged = false;

        // BGM音量
        if (listCursor == 0) {
          config.bgmVolume += change; // ctx->config ではなく 引数の config を使う
          if (config.bgmVolume < 0) config.bgmVolume = 0;
          if (config.bgmVolume > 10) config.bgmVolume = 10;
          
          // 即座に反映 (ctx->music ではなく 引数の music を使う)
          music.bgmVolume = (float)config.bgmVolume / 10.0f; 
          valChanged = true;
        }
        // SE音量
        else if (listCursor == 1) {
          config.seVolume += change;
          if (config.seVolume < 0) config.seVolume = 0;
          if (config.seVolume > 10) config.seVolume = 10;
          music.setSeVolume(config.seVolume);
          // SE音量を反映 (MUSICクラスに変数を持たせるか、playSEで計算)
          // music.setSeVolume(config.seVolume); // 推奨
          
          valChanged = true;
          if(valChanged) music.playSE(0); // 音量確認
        }
        // 振動 ON/OFF
        else if (listCursor == 2) {
          config.vibration = !config.vibration;
          valChanged = true;
          vib.setEnabled(config.vibration);
          // 振動確認 (ctx->vibration ではなく 引数の vib を使う)
          if(config.vibration) vib.trigger(100); 
        }
      }
      
      // Aボタン or Bボタンで戻る
      if (aPressed && listCursor == 3) {
        music.playSE(1);
        currentMenuState = STATE_MAIN;
      }
      break;
    }
  } // switch 終了
}

// main.ino (setup 関数)

void setup() {
  Serial.begin(115200);

  critical_section_init(&sdLock);
  
  // シード設定
  uint32_t seed = micros() ^ time_us_32();
  for (int i = 0; i < 8; ++i) {
    seed ^= (analogRead(A2) + (micros() & 0xFFFF)) << (i & 31);
    delay(1);
  }
  randomSeed(seed);
  // 1. 基本オブジェクトの生成
  ctx = new GameContext();
  ctx->graphic = new Graphic();
  ctx->controller = new Controller();
  ctx->music = new MUSIC();
  ctx->sd = new Sd();
  ctx->imgMgr = new ImageManager();
  // Graphic, SD の初期化
  ctx->graphic->init();
  ctx->sd->init(*ctx->music);
  // SaveManager 生成
  ctx->saveManager = new SaveManager();
  ctx->vibration = new Vibration();
  ctx->vibration->init();

  ctx->music->bgmVolume = (float)ctx->config.bgmVolume / 10.0f;
  ctx->music->setSeVolume(ctx->config.seVolume);

  multicore_launch_core1(core1_main);
  delay(200);

  ctx->music->switchTrack(9, *ctx->sd);
  // 2. ★★★ タイトル画面処理 (ラインスプライト版) ★★★
  bool loadSuccess = false;
  bool startNewGame = false;
  int titleCursor = 0;
  
  // セーブデータがあるか確認
  bool hasSave = ctx->saveManager->hasSaveFile(*ctx->sd);
  if (!hasSave) titleCursor = 0; // なければ「はじめから」固定
  else titleCursor = 1;          // あれば「つづきから」をデフォルトに

  // フォント設定 (lineSprite用)
  ctx->graphic->lineSprite.setFont(&fonts::lgfxJapanGothic_12);

  while (!startNewGame && !loadSuccess) {
    // 入力取得
    bool up = ctx->controller->pressedDebounced(ctx->controller->BTN_UP);
    bool down = ctx->controller->pressedDebounced(ctx->controller->BTN_DOWN);
    bool a = ctx->controller->pressedDebounced(ctx->controller->BTN_A);

    if (up || down) {
      if (hasSave) titleCursor = (titleCursor == 0) ? 1 : 0;
      ctx->music->playSE(2);
    }

    if (a) {
      ctx->music->playSE(0);
      if (titleCursor == 0) {
        startNewGame = true;
        ctx->vibration->setEnabled(ctx->config.vibration);
      } else {
        // ロード用の仮オブジェクト生成
        ctx->map = new Map(20, 20, Map::TYPE_TOWN); 
        ctx->hero = new Caractor(*ctx->map, 1, 38); 
        ctx->inventory = new Inventory();
        ctx->party = new Party();
        
        if (ctx->saveManager->loadGame(*ctx->sd, *ctx->music, *ctx->map, *ctx->hero, *ctx->party, *ctx->inventory, ctx->bossDefeated)) {
          loadSuccess = true;
          ctx->vibration->setEnabled(ctx->config.vibration);
        } else {
          ctx->music->playSE(1);
          startNewGame = true; // ロード失敗時は新規
          ctx->vibration->setEnabled(ctx->config.vibration);
        }
      }
    }

    // ★★★ ラインスプライトによる分割描画 ★★★
    for (int drawY = 0; drawY < Graphic::SCREEN_HEIGHT; drawY += Graphic::BLOCK_HEIGHT) {
      ctx->graphic->lineSprite.fillScreen(TFT_BLACK);
      
      // タイトルロゴ
      ctx->graphic->lineSprite.setTextSize(3);
      ctx->graphic->lineSprite.setTextColor(TFT_WHITE);
      ctx->graphic->lineSprite.setCursor(60, 80 - drawY); // ★ Y座標を drawY 分ずらす
      ctx->graphic->lineSprite.print("RPG GAME");

      // メニュー項目
      ctx->graphic->lineSprite.setTextSize(2);
      
      // 「はじめから」
      ctx->graphic->lineSprite.setTextColor(titleCursor == 0 ? TFT_YELLOW : TFT_WHITE);
      ctx->graphic->lineSprite.setCursor(100, 150 - drawY); // ★
      ctx->graphic->lineSprite.print("はじめから");

      // 「つづきから」
      if (hasSave) ctx->graphic->lineSprite.setTextColor(titleCursor == 1 ? TFT_YELLOW : TFT_WHITE);
      else ctx->graphic->lineSprite.setTextColor(TFT_DARKGREY);
      ctx->graphic->lineSprite.setCursor(100, 180 - drawY); // ★
      ctx->graphic->lineSprite.print("つづきから");

      // 画面に転送
      ctx->graphic->lineSprite.pushSprite(0, drawY);
    }
  }

  // 3. ゲーム開始準備
  if (startNewGame) {
    // 新規作成
    if (ctx->map) delete ctx->map;
    if (ctx->hero) delete ctx->hero;
    if (ctx->inventory) delete ctx->inventory;
    if (ctx->party) delete ctx->party; // (中でAutomaton->Statusも消える)
    ctx->map = new Map(20, 20, Map::TYPE_TOWN);
    ctx->map->currentFloor = 1;
    ctx->map->isTown = true;
    
    ctx->hero = new Caractor(*ctx->map, 1, 38); // Lv1
    ctx->hero->X = ctx->map->playerStartPos.x * Map::TILE_SIZE;
    ctx->hero->Y = ctx->map->playerStartPos.y * Map::TILE_SIZE;
    ctx->inventory = new Inventory();
    // 初期アイテム
    ctx->inventory->addItem(createItemById(100)); // リペアキット
    
    ctx->party = new Party();
    Automaton* firstAlly = new Automaton(1, 39);
    ctx->party->addMember(firstAlly);
  }

  // 共通初期化
  ctx->menu = new Menu();
  ctx->encounter = new Encounter();
  ctx->battle = new Battle();

  // Graphicクラスへ階層通知
  ctx->graphic->setFloor(ctx->map->currentFloor);

  // 初回描画 (ここもGraphicクラスに任せるので安全)
  int camX = ctx->hero->X - Graphic::SCREEN_WIDTH / 2;
  int camY = ctx->hero->Y - Graphic::SCREEN_HEIGHT / 2;
  ctx->graphic->drawFullBackground(*ctx->map, *ctx->hero, camX, camY);
  if (ctx->map->isTown) {
    // 集落なら bgm_0.raw
    ctx->music->switchTrack(0, *ctx->sd); 
  } else {
    // ダンジョンなら bgm_1.raw
    ctx->music->switchTrack(1, *ctx->sd);
  }

  // ★★★ 追加: ゲーム状態への移行 ★★★
  ctx->state = STATE_GAME;
  // 4. 音楽スレッド起動
}

void loop() {
  ctx->memMonitor.update();
  if (ctx->requestRegenerate) {
    ctx->requestRegenerate = false;

    // MapとHeroの内容だけ再初期化（new/deleteなし）
    noInterrupts();
    ctx->map->regenerate(20, 20, Map::TYPE_DUNGEON);
    ctx->hero->reset(*ctx->map);

    ctx->graphic->setFloor(ctx->map->currentFloor);

    int cameraX = ctx->hero->X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = ctx->hero->Y - Graphic::SCREEN_HEIGHT / 2;
    ctx->graphic->invalidateAndRedraw(*ctx->map, *ctx->hero, cameraX, cameraY);
    interrupts();
  }
  static GameState prevState = STATE_GAME;
  static int gameOverCursor = 0;
  auto& g = *ctx->graphic;
  auto& map = *ctx->map;
  auto& hero = *ctx->hero;
  auto& ctrl = *ctx->controller;
  auto& menu = *ctx->menu;
  auto& encounter = *ctx->encounter;
  auto& inventory = *ctx->inventory;
  auto& party = *ctx->party;
  auto& state = ctx->state;
  auto& music = *ctx->music;
  auto& sd = *ctx->sd;
  auto& currentEvent = ctx->currentEvent;
  auto& save = *ctx->saveManager;
  auto& vib = *ctx->vibration;
  auto& imgmgr = *ctx->imgMgr;
  auto& conf = ctx->config;
  vib.update();
  if (state == STATE_GAME) {
    hero.oldX = hero.X;
    hero.oldY = hero.Y;

    int prevX = hero.X, prevY = hero.Y;
    ctrl.update_cara_point(hero);
    if (ctrl.pressed(ctrl.BTN_UP))    hero.direction = Caractor::DIR_UP;
    if (ctrl.pressed(ctrl.BTN_DOWN))  hero.direction = Caractor::DIR_DOWN;
    if (ctrl.pressed(ctrl.BTN_LEFT))  hero.direction = Caractor::DIR_LEFT;
    if (ctrl.pressed(ctrl.BTN_RIGHT)) hero.direction = Caractor::DIR_RIGHT;
    ctrl.check_wall(hero, map);

    bool eventOccurred = false;
    if (ctrl.pressedDebounced(ctrl.BTN_A)){
      eventOccurred = ctrl.check_event(hero, map, state, currentEvent);
    }
    // 2. 移動が発生したかチェック
    bool moved = (hero.X != prevX || hero.Y != prevY);
    // 3. 移動していて、メニューが開いていない場合のみイベントとエンカウントをチェック
    if (moved && !menu.active) {
      // 5. イベントが発生しなかった場合のみ、エンカウントをチェック
      if (!eventOccurred && !map.isTown) {
        if (!map.isTown && map.currentType != Map::MapType::TYPE_BOSS_ROOM && encounter.checkEncounter()) {
          vib.trigger(300);
          state = STATE_BATTLE;
          ctx->battle->start(music, sd, hero, party, map.currentFloor, imgmgr, map);
        }
      }
    }

    // 置換箇所（STATE_GAME 内）
    int cameraX = hero.X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = hero.Y - Graphic::SCREEN_HEIGHT / 2;
    // map が再生成されたときは drawFullBackground を呼んでいるので通常は smartRender だけでOK
    g.smartRender(map, hero, cameraX, cameraY);


    if (ctrl.pressed(ctrl.BTN_X)) {
      menu.toggle();
      if (menu.active) state = STATE_MENU;
    }
  }

  else if (state == STATE_MENU) {
    //menu.update(ctrl, inventory, hero, party);
    menu.update(ctrl, inventory, hero, party, map, music, sd, save, conf, vib, ctx->bossDefeated);
    //g.screenSprite.fillScreen(TFT_BLACK);
    menu.draw(&g, inventory, hero, party, conf);
    //g.screenSprite.pushSprite(0,0);

    if (!menu.active) {
      state = STATE_GAME; // ゲーム画面に戻る
      //もしうまくいかなかったらここを
      ctx->requestRegenerate = false;
    }
  }

  else if (state == STATE_BATTLE) {
    auto &battle = *ctx->battle;
    battle.update(ctrl, state, music, sd, hero, inventory, vib, map, g, ctx->bossDefeated);
    battle.draw(&g);
    //g.screenSprite.pushSprite(0, 0);
  }

  else if (state == STATE_EVENT) {
    
    // ★★★ ここから修正 ★★★
    
    // イベントの種類に応じて処理を分岐
    switch (currentEvent) {
      
      case EVENT_TREASURE_BOX: {
        // --- 宝箱イベント ---
          
          // 2a. プレイヤーの現在地を取得
        int heroTileX = (hero.X + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
        int heroTileY = (hero.Y + Map::TILE_SIZE / 2) / Map::TILE_SIZE;

        // 2b. マップデータを「開いた」状態に更新
        map.mapData[heroTileY * map.getMAP_WIDTH() + heroTileX] = Map::TILE_BOX_OPEN;
          
        // 2. 中身の抽選
        int floor = map.currentFloor;
        int itemId = 0;
        int r = random(100);

        // --- 確率設定 ---
        // 45%: 消費道具
        // 30%: 素材
        // 15%: レア素材
        // 3%:  旋律の巻物
        // 7%:  装備品 (武器/防具/アクセ) ★ NEW!

        if (r < 45) { // 消費道具
          if (floor <= 10) itemId = 100;      // リペアキット
          else if (floor <= 30) itemId = 103; // エネルギー缶
          else itemId = 101;                  // リペアキット中
        } 
        else if (r < 75) { // 素材
          itemId = 2; // 銅の歯車
        }
        else if (r < 90) { // レア素材
          if (floor <= 20) itemId = 1; // 鉄くず(進化素材)
          else if (floor <= 30) itemId = 10 + random(0, 2); // 銀/金の歯車
          else itemId = 13; // 謎のパーツ
        }
        else if (r < 93) { // 巻物 (3%)
          int minSkill = 1, maxSkill = 5;
          if (floor > 10) { minSkill=6; maxSkill=11; }
          if (floor > 30) { minSkill=12; maxSkill=13; }
          itemId = 200 + random(minSkill, maxSkill + 1);
        }
        else { // ★★★ 装備品 (7%) ★★★
          int tier = (floor - 1) / 10; // 0～4
          if (tier > 4) tier = 4;
          
          // 装備カテゴリ抽選 (0:武器, 1:防具, 2:アクセ)
          int typeR = random(0, 3); 
          
          // ★ 激レア判定 (さらに10%の確率で最強装備)
          bool isRare = (random(100) < 10);

          if (typeR == 0) { // 武器
            if (isRare && floor >= 30) itemId = 399; // 創世のタクト
            else itemId = 300 + tier; // 階層に応じた武器
          } 
          else if (typeR == 1) { // 防具
            if (isRare && floor >= 30) itemId = 499; // イージス・ギア
            else itemId = 400 + tier; 
          } 
          else { // アクセ
            if (isRare && floor >= 30) itemId = 599; // トリニティ・コア
            else itemId = 500 + random(0, 5); // 500-504からランダム
          }
        }

        // 3. アイテム入手
        Item foundItem = createItemById(itemId);
        inventory.addItem(foundItem);
        
        // (メッセージ表示などは描画システムの都合上省略、あるいはフラグを立てて別途表示)
          
        // 2d. 即座にゲームに戻る (待機処理は行わない)
        state = STATE_GAME;
        currentEvent = EVENT_NONE; // イベントタイプをリセット
        break; // EVENT_TREASURE_BOX 終了
      }

      case EVENT_STAIR_UP: {
        // 50Fを最終階とする
        if (map.currentFloor >= 50) {
          state = STATE_GAME;
          currentEvent = EVENT_NONE;
          break;
        }

        map.currentFloor++;

        int width = 0;
        int height = 0;

        Map::MapType nextType =
            Map::MapType::TYPE_DUNGEON;

        // 10F / 20F / 30F / 40F / 50F
        if (map.currentFloor % 10 == 0) {

          int bossIndex =
              (map.currentFloor / 10) - 1;

          // bossDefeated[] の範囲内だけ参照する
          if (bossIndex >= 0 &&
              bossIndex < 5)
          {
            width = height = 5;

            if (ctx->bossDefeated[bossIndex]) {
              // 撃破済みなら集落
              nextType = Map::MapType::TYPE_TOWN;
              map.isTown = true;
              music.switchTrack(0, sd);
            }
            else {
              // 未撃破ならボス部屋
              nextType = Map::MapType::TYPE_BOSS_ROOM;
              map.isTown = false;
              music.switchTrack(1, sd);
            }
          }
          else {
            // 想定外の10の倍数階
            width = height = 20;
            nextType = Map::MapType::TYPE_DUNGEON;
            map.isTown = false;
            music.switchTrack(1, sd);
          }
        }
        else {
          // 通常階
          width = height = 20;
          nextType = Map::MapType::TYPE_DUNGEON;
          map.isTown = false;
          music.switchTrack(1, sd);
        }

        ctx->graphic->setFloor(map.currentFloor);

        ctx->map->regenerate(
            width,
            height,
            nextType
        );

        ctx->hero->reset(*ctx->map);

        state = STATE_GAME;
        currentEvent = EVENT_NONE;
        break;
      }

      case EVENT_STAIR_DOWN: {
        if (map.currentFloor > 1) {

          map.currentFloor--;

          int width = 0;
          int height = 0;

          Map::MapType nextType =
              Map::MapType::TYPE_DUNGEON;

          // 1Fは集落
          if (map.currentFloor == 1) {

            nextType = Map::MapType::TYPE_TOWN;
            map.isTown = true;
            music.switchTrack(0, sd);
          }

          // 10F / 20F / 30F / 40F / 50F
          else if (map.currentFloor % 10 == 0) {

            int bossIndex =
                (map.currentFloor / 10) - 1;

            // bossDefeated[] の範囲内だけ参照
            if (bossIndex >= 0 &&
                bossIndex < 5)
            {
              width = height = 5;

              if (ctx->bossDefeated[bossIndex]) {
                // 撃破済みなら集落
                nextType = Map::MapType::TYPE_TOWN;
                map.isTown = true;
                music.switchTrack(0, sd);
              }
              else {
                // 未撃破ならボス部屋
                nextType = Map::MapType::TYPE_BOSS_ROOM;
                map.isTown = false;
                music.switchTrack(1, sd);
              }
            }
            else {
              // 想定外の10の倍数階
              width = height = 20;
              nextType = Map::MapType::TYPE_DUNGEON;
              map.isTown = false;
              music.switchTrack(1, sd);
            }
          }

          // 通常階
          else {
            width = height = 20;
            nextType = Map::MapType::TYPE_DUNGEON;
            map.isTown = false;
            music.switchTrack(1, sd);
          }

          ctx->graphic->setFloor(map.currentFloor);

          ctx->map->regenerate(
              width,
              height,
              nextType
          );

          ctx->hero->reset(*ctx->map);
        }

        state = STATE_GAME;
        currentEvent = EVENT_NONE;
        break;
      }

      case EVENT_AUTOMATON: {
        // TODO: 「壊れたオートマタがある。調律しますか？」の
        // メッセージウィンドウを本当は出すべき

        // (今はAボタンで即座に仲間になる)
        // (Aボタンは check_event の呼び出し元で押されている)

        // TODO: 今は who=39 (初号機) で固定。
        // 将来はフロア番号やマップの保持データから who ID を決定する
        int who_to_add = map.automatonWhoId;
        Automaton* newAlly = new Automaton(1, who_to_add); // 主人公と同じレベルで加入
        
        int result = party.addMember(newAlly);
        
        if (result == 1) {
            // パーティーに入った
        } else if (result == 2) {
            // 預かり所へ送られた (自動)
        }
        
        // タイルを消す
        int heroTileX = (hero.X + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
        int heroTileY = (hero.Y + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
        map.mapData[heroTileY * map.getMAP_WIDTH() + heroTileX] = Map::TILE_GRASS;
        
        state = STATE_GAME;
        currentEvent = EVENT_NONE;
        break;
      }
      case EVENT_INN: {
        ctx->menu->active = true;
        ctx->menu->currentMenuState = Menu::STATE_INN;
        ctx->menu->listCursor = 0; // 0:はい, 1:いいえ
        state = STATE_MENU; // ゲーム画面からメニュー画面へ
        currentEvent = EVENT_NONE; // イベントフラグは解除してOK
        break;
      }
      // --- 預かり所 ---
      case EVENT_STORAGE: {
        // メニューの預かり所モードを起動
        menu.active = true;
        menu.currentMenuState = Menu::STATE_STORAGE;
        menu.storageMode = 0;
        menu.listCursor = 0;
        
        state = STATE_MENU; // メニュー画面へ
        currentEvent = EVENT_NONE;
        break;
      }
      // --- ショップ (後回し、とりあえずダミー) ---
      case EVENT_SHOP: {
        ctx->menu->active = true;
        ctx->menu->currentMenuState = Menu::STATE_SHOP_CHOICE;
        ctx->menu->listCursor = 0; // 0:買う, 1:売る, 2:出る
        state = STATE_MENU; 
        currentEvent = EVENT_NONE;
        break;
      }

      case EVENT_BOSS_BATTLE: {

        int bossIndex =
            (map.currentFloor / 10) - 1;

        // 撃破済みボスのタイルが何らかの理由で残っていた場合
        if (map.currentFloor >= 10 &&
            map.currentFloor <= 50 &&
            map.currentFloor % 10 == 0 &&
            bossIndex >= 0 &&
            bossIndex < 5 &&
            ctx->bossDefeated[bossIndex])
        {
          // ボスを復活させず、集落へ補正する
          map.regenerate(
              20,
              20,
              Map::TYPE_TOWN
          );

          map.isTown = true;
          hero.reset(map);

          music.switchTrack(0, sd);

          state = STATE_GAME;
          currentEvent = EVENT_NONE;
          break;
        }

        // 未撃破なら通常どおりボス戦
        vib.trigger(500);

        state = STATE_BATTLE;

        ctx->battle->start(
            music,
            sd,
            hero,
            party,
            map.currentFloor,
            imgmgr,
            map
        );

        currentEvent = EVENT_NONE;
        break;
      }

      case EVENT_NONE:
      default:
        // 何らかのバグでイベントタイプが未設定の場合
        state = STATE_GAME; // とりあえずゲームに戻る
        break;
    }
    // ★★★ 修正ここまで ★★★
  }

  else if (state == STATE_GAME_OVER) {
    // -------------------------------------------------
    // 1. 入力処理 (描画ループの外に出す)
    // -------------------------------------------------
    // 描画中にボタン判定をすると重くなるので、必ずループの外で行います
    if (ctrl.pressedDebounced(ctrl.BTN_A)) {
      ctx->music->playSE(0); 
      // リセット
      rp2040.reboot();
      while (1) {
        // 何もしないで死を待つ
      }
      return;
    }

    // -------------------------------------------------
    // 2. 描画処理 (ラインスプライト方式)
    // -------------------------------------------------
    // 画面上部(0)から下部(SCREEN_HEIGHT)まで、ブロック単位でループ
    for (int drawY = 0; drawY < Graphic::SCREEN_HEIGHT; drawY += Graphic::BLOCK_HEIGHT) {
      
      // (1) スプライトをクリア
      ctx->graphic->lineSprite.fillScreen(TFT_BLACK);

      // (2) 文字描画設定
      // 左上(Top-Left)を基準点にします。これがないと位置がずれることがあります。
      ctx->graphic->lineSprite.setTextDatum(TL_DATUM); 

      // (3) "GAME OVER" 描画
      ctx->graphic->lineSprite.setTextColor(TFT_RED);
      ctx->graphic->lineSprite.setTextSize(3);
      // print ではなく drawString を使います
      // Y座標: 「本来のY座標(80) - 現在の描画開始位置(drawY)」
      ctx->graphic->lineSprite.drawString("GAME OVER", 60, 80 - drawY);

      // (4) "タイトルへ" 描画
      ctx->graphic->lineSprite.setTextSize(2);
      ctx->graphic->lineSprite.setTextColor(TFT_YELLOW);
      // Y座標: 「本来のY座標(150) - 現在の描画開始位置(drawY)」
      ctx->graphic->lineSprite.drawString("タイトルへ", 80, 150 - drawY);

      // (5) 画面への転送
      // 作った短冊を、画面上の drawY の位置に貼り付けます
      ctx->graphic->lineSprite.pushSprite(0, drawY);
    }
  }

  if (prevState != state) {
    if (state == STATE_GAME) {
      // メニュー／戦闘等からゲーム画面に戻った：背景を一括再描画してヒーローを正常復帰
      int cameraX = hero.X - Graphic::SCREEN_WIDTH / 2;
      int cameraY = hero.Y - Graphic::SCREEN_HEIGHT / 2;
      g.invalidateAndRedraw(map, hero, cameraX, cameraY);
    } else {
      // メニュー／戦闘へ入るときは「画面を上書きする」ので
      // prevHero を無効化して、restore が誤動作しないようにする
      g.prevHeroScreenX = -1;
      g.prevHeroScreenY = -1;
    }
    prevState = state;
  }
}