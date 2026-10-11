// main.ino
#include <SPI.h>
#include <SdFat.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "bmp.h"
#include "Item.h"
#include "Status.h"
#include "Automaton.h"
#include "Party.h"
#include "Enemy.h"
#include "Caractor.h"
#include "Graphic.h"
#include "Skill.h"
#include "Inventory.h"
#include "Map.h"
#include "MemoryMonitor.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "pico/sync.h"
critical_section_t sdLock;

#define LGFX_USE_SD 1
#define H_TRANSPARENT 0xFFFF
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





// main.ino

Item createItemById(int id) {
  
  // --- 0. 素材・換金アイテム (0-99) ---
  
  // 基本素材
  if (id == 1) return Item(1, "鉄くず", Item::ITEM_MATERIAL, 0, 10);
  if (id == 2) return Item(2, "銅の歯車", Item::ITEM_MATERIAL, 0, 30);
  
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









// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------



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
      // 押下状態（LOW）をboolで保持
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
  void check_wall(Caractor &hero, Map &map) {
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

  bool check_event(Caractor &hero, Map &map, GameState &state, EventType &currentEvent) {
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
  // SEシンセ用の状態
  
  // 波形タイプ
  enum WaveType { OFF, SQUARE, NOISE };
  
  volatile WaveType seType = OFF; // 現在鳴っている音の種類
  volatile float sePhase = 0;     // 波形の進行度
  volatile float seFreq = 0;      // 周波数 (高さ)
  volatile float seFreqSlide = 0; // 周波数の変化量 (ピュイッ、のような効果)
  volatile float seVol = 0;       // 音量
  volatile float seVolDecay = 0;  // 音量の減衰量 (フェードアウト)
  volatile int targetSeVol = 10;
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
    seType = OFF;
    sePhase = 0;
    seFreq = 0;
    seFreqSlide = 0;
    seVol = 0;
    seVolDecay = 0;
  }
  // MUSIC クラス内
  
  void playSE(int id) {
      sePhase = 0;
      // 設定値(0-10)を反映
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
    // 補充要求がないなら何もしない
    if (!music.needs_refill) {
      return;
    }

    // 曲切り替え中などはSD読み込みしない
    if (music.paused) {
      return;
    }

    FsFile& file =
        music.musicFiles[music.currentTrack];

    if (!file.isOpen()) {
      return;
    }

    critical_section_enter_blocking(&sdLock);

    // 一度に長時間SDを占有しないよう、
    // 2048 byteずつ裏バッファへ補充する
    const size_t CHUNK = 2048;

    if (refillOffset < MUSIC::BUF_SIZE) {

      size_t remaining =
          MUSIC::BUF_SIZE - refillOffset;

      size_t toRead =
          min(CHUNK, remaining);

      size_t n =
          file.read(
              (void*)(music.fillBuf + refillOffset),
              toRead
          );

      refillOffset += n;

      // 要求量より少なくしか読めなかった場合は
      // 曲末尾に到達したとみなし、次回は先頭から続ける。
      //
      // 途中まで読めたデータは捨てず、
      // fillBuf内にそのまま残しておく。
      if (n < toRead) {
        file.seek(0);
      }
    }

    critical_section_exit(&sdLock);

    // 裏バッファ1個分が完成したら再生側へ公開
    if (refillOffset >= MUSIC::BUF_SIZE) {

      noInterrupts();

      music.fillBytes =
          MUSIC::BUF_SIZE;

      music.needs_refill =
          false;

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
    file.println(map.getFloor());
    file.println((int)map.getType());

    file.println(map.getMAP_WIDTH());
    file.println(map.getMAP_HEIGHT());

    // マップ上の未回収オートマタの種類
    file.println(map.getAutomatonWhoId());

    // 主人公の現在位置・向き
    file.println(hero.X);
    file.println(hero.Y);
    file.println((int)hero.direction);

    // マップそのもの
    file.println(
        map.getMAP_WIDTH() *
        map.getMAP_HEIGHT()
    );

    for (int y = 0; y < map.getMAP_HEIGHT(); y++) {
      for (int x = 0; x < map.getMAP_WIDTH(); x++) {
        file.println((int)map.getTile(x, y));
      }
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

    firstLine.trim();

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
      map.restoreFloor(
          file.readStringUntil('\n').toInt()
      );

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

      map.restoreAutomatonWhoId(
          file.readStringUntil('\n').toInt()
      );

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

      map.beginRestoreFromSave(
          loadedType,
          savedWidth,
          savedHeight
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

        int x = i % savedWidth;
        int y = i / savedWidth;

        map.setTile(
            x,
            y,
            (uint8_t)tile
        );
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
      map.restoreFloor(
          firstLine.toInt()
      );

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
      if (map.getFloor() >= 10 &&
          map.getFloor() <= 50 &&
          map.getFloor() % 10 == 0)
      {
        int bossIndex =
            (map.getFloor() / 10) - 1;

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
    // ショップ・宿屋
    STATE_INN,          // 宿屋 (泊まりますか？)
    STATE_SHOP_CHOICE,  // 店 (買う/売る/出る)
    STATE_SHOP_BUY,     // 買う画面
    STATE_SHOP_SELL,     // 売る画面
    STATE_CONFIG
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

      int counterDamage =
          defender->getAttack();

      counterDamage =
          attacker->takedamage(
              counterDamage,
              Skill::Category::PHYSICAL
          );

      attacker->addPopup(
          counterDamage,
          false
      );
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
    
    // 6. 行動前・継続効果処理
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
    for (auto enemy : activeEnemies) {
      if (enemy) delete enemy;
    }
    activeEnemies.clear();
    activeEnemies.reserve(4);
    if (floor >= 10 &&
        floor <= 50 &&
        floor % 10 == 0 &&
        !map.isTown())
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

      // 0. ターン開始処理
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

      // 1. 行動者を決定する
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
      
      // 5. ターゲット選択 (味方)
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

      // 6. 行動前・継続効果処理
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
          // オートリバイブ判定
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

        // 毒・麻痺・混乱などの状態異常を処理
        currentBattleState = STATE_PRE_ACTION_EFFECTS;

        break;
      }

      // 10. 行動の実行（計算）
      case STATE_ACTION_CALC: {
        lastDamage = 0;
        lastActionMessage = "";

        messageFlow = MessageFlow::ACTION_COMPLETE;
        // ポップアップを安全にクリア
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
          
          // 戦闘終了時にステータスをリセット
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
            int bossIndex = (map.getFloor() / 10) - 1;
            if (bossIndex >= 0 && bossIndex < 5) {
              bossFlags[bossIndex] = true;
            }

            // マップを集落に作り変える
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
            if (map.isTown()) music.switchTrack(0, sd);
            else music.switchTrack(1, sd);
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
                // 会心なら赤、通常なら黄色
                if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                else g->lineSprite.setTextColor(TFT_YELLOW);
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
                  // 会心なら赤、通常なら黄色
                  if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                  else g->lineSprite.setTextColor(TFT_YELLOW);
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
        // メッセージをキャラと重ならない位置に表示
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
    int actualDamage = 0;
    bool isCrit = false;
    switch (skill.type) {
      
      case Skill::EffectType::DAMAGE:
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
        actualDamage =
            target->takedamage(value, skill.category);

        target->addPopup(actualDamage, isCrit);
        lastDamage += actualDamage;
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
            actualDamage =
                target->takedamage(value, skill.category);

            target->addPopup(actualDamage, isCrit);
            lastDamage += actualDamage;
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

            actualDamage =
                target->takedamage(dmg, skill.category);

            target->addPopup(actualDamage, true);

            if (target->getHp() <= 0) {
              lastActionMessage = "急所を突いた! 即死!";
            }
            else {
              // バリア、アダマン・ボディ、ラストスタンドなどで
              // 即死を防がれた場合
              lastActionMessage = "急所を突いた!";
            }
          } else {
            // 失敗時は通常ダメージの半分
            music.playSE(5);
            value = user->caclulateSkillDamage(target, skill, isCrit) / 2;
            if(value<1) value=1;
            actualDamage =
                target->takedamage(value, skill.category);

            target->addPopup(actualDamage, isCrit);
            lastActionMessage = "急所を外した...";
          }
        }

        // 3. ペインサイクル / TPドレイン (吸収)
        else if (skill.id == 44 || skill.id == 111) {
          if (skill.id == 44) { // HP吸収
            music.playSE(5);
            value =
                user->caclulateSkillDamage(
                    target,
                    skill,
                    isCrit
                );

            actualDamage =
                target->takedamage(value, skill.category);

            target->addPopup(actualDamage, isCrit);

            // 実際に与えたダメージ分だけ吸収
            user->ReceiveHeal(actualDamage);
            user->addPopup(-actualDamage, false);
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

          actualDamage =
              target->takedamage(value, skill.category);

          target->addPopup(actualDamage, isCrit);

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

          actualDamage =
              target->takedamage(value, skill.category);

          target->addPopup(actualDamage, isCrit);

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

          actualDamage =
              target->takedamage(value, skill.category);

          target->addPopup(actualDamage, isCrit);

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

          actualDamage =
              target->takedamage(value, skill.category);

          target->addPopup(actualDamage, isCrit);

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

struct FloorMapConfig {
  int width;
  int height;
  Map::MapType type;
  int bgmTrack;
};

FloorMapConfig getFloorMapConfig(
    int floor,
    const bool bossDefeated[5]);

void applyFloorMapConfig(GameContext* ctx);

FloorMapConfig getFloorMapConfig(
    int floor,
    const bool bossDefeated[5])
{
  // 1Fは集落
  if (floor == 1) {
    return {
      0,
      0,
      Map::TYPE_TOWN,
      0
    };
  }

  // 10F / 20F / 30F / 40F / 50F
  if (floor % 10 == 0) {
    int bossIndex = (floor / 10) - 1;

    if (bossIndex >= 0 && bossIndex < 5) {
      if (bossDefeated[bossIndex]) {
        return {
          5,
          5,
          Map::TYPE_TOWN,
          0
        };
      }

      return {
        5,
        5,
        Map::TYPE_BOSS_ROOM,
        1
      };
    }
  }

  // 通常階
  return {
    20,
    20,
    Map::TYPE_DUNGEON,
    1
  };
}

void applyFloorMapConfig(GameContext* ctx)
{
  FloorMapConfig config =
      getFloorMapConfig(
          ctx->map->getFloor(),
          ctx->bossDefeated
      );

  ctx->music->switchTrack(
      config.bgmTrack,
      *ctx->sd
  );

  ctx->graphic->setFloor(
      ctx->map->getFloor()
  );

  ctx->map->regenerate(
      config.width,
      config.height,
      config.type
  );

  ctx->hero->reset(*ctx->map);
}

// タイマー割り込み関数

bool audio_timer_callback(struct repeating_timer *t) {
  MUSIC* music = ctx->music;
  
  // BGM停止中でもSEは鳴らす
  int32_t mixedSample = 0;

  // --- 1. BGMの取得 ---
  if (!music->paused) {

    // 現在の再生バッファを使い切っていたら、
    // 先に裏バッファへ切り替える
    if (music->sampleIndex + 1 >= music->playBytes) {

      if (music->fillBytes > 0) {

        // ここはタイマーISR内なので、
        // noInterrupts()/interrupts() は不要
        volatile uint8_t* tmp = music->playBuf;
        music->playBuf = music->fillBuf;
        music->fillBuf = (uint8_t*)tmp;

        music->playBytes = music->fillBytes;
        music->fillBytes = 0;
        music->sampleIndex = 0;

        // 空いた裏バッファの補充をcore1ループへ要求
        music->needs_refill = true;
      }
      else {
        // 裏バッファの準備が間に合っていない
        music->needs_refill = true;
      }
    }

    // バッファを切り替えた場合も、
    // 同じタイマー割り込み内でそのまま次の1サンプルを読む。
    // これによりバッファ境界に無音サンプルを挟まない。
    if (music->sampleIndex + 1 < music->playBytes) {

      uint8_t lo =
          music->playBuf[music->sampleIndex];

      uint8_t hi =
          music->playBuf[music->sampleIndex + 1];

      int16_t bgm =
          (int16_t)(lo | (hi << 8));

      bgm =
          (int16_t)(bgm * music->bgmVolume);

      mixedSample += bgm;

      music->sampleIndex += 2;
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
      // 全アイテムを対象にする
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
          if (map.getFloor() >= 10) shopItemList.push_back(101); // リペアキット中

          // 2. 階層に応じた装備品 (Tier計算)
          // 1-10F: Tier 0, 11-20F: Tier 1, ...
          int tier = (map.getFloor() - 1) / 10;
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
    
    ctx->hero = new Caractor(*ctx->map, 1, 38);

    Map::ObjectPos startPos =
        ctx->map->getPlayerStartPos();

    ctx->hero->X =
        startPos.x * Map::TILE_SIZE;

    ctx->hero->Y =
        startPos.y * Map::TILE_SIZE;

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
  ctx->graphic->setFloor(ctx->map->getFloor());

  // 初回描画 (ここもGraphicクラスに任せるので安全)
  int camX = ctx->hero->X - Graphic::SCREEN_WIDTH / 2;
  int camY = ctx->hero->Y - Graphic::SCREEN_HEIGHT / 2;
  ctx->graphic->drawFullBackground(*ctx->map, *ctx->hero, camX, camY);
  if (ctx->map->isTown()) {
    // 集落なら bgm_0.raw
    ctx->music->switchTrack(0, *ctx->sd); 
  } else {
    // ダンジョンなら bgm_1.raw
    ctx->music->switchTrack(1, *ctx->sd);
  }

  // ゲーム状態へ移行
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

    ctx->graphic->setFloor(ctx->map->getFloor());

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
      if (!eventOccurred && !map.isTown()) {
        if (!map.isBossRoom() &&
            encounter.checkEncounter()) {
          vib.trigger(300);
          state = STATE_BATTLE;
          ctx->battle->start(music, sd, hero, party, map.getFloor(), imgmgr, map);
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
    
    
    
    // イベントの種類に応じて処理を分岐
    switch (currentEvent) {
      
      case EVENT_TREASURE_BOX: {
        // --- 宝箱イベント ---
          
          // 2a. プレイヤーの現在地を取得
        int heroTileX = (hero.X + Map::TILE_SIZE / 2) / Map::TILE_SIZE;
        int heroTileY = (hero.Y + Map::TILE_SIZE / 2) / Map::TILE_SIZE;

        // 2b. マップデータを「開いた」状態に更新
        map.setTile(heroTileX, heroTileY, Map::TILE_BOX_OPEN);
          
        // 2. 中身の抽選
        int floor = map.getFloor();
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
          if (floor <= 20) {
            itemId = 1; // 鉄くず
          }
          else if (floor <= 30) {
            itemId = 3 + random(0, 2); // 銀の歯車 / 金の歯車
          }
          else {
            itemId = 5; // 謎のパーツ
          }
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
        if (map.advanceFloor()) {
          applyFloorMapConfig(ctx);
        }

        state = STATE_GAME;
        currentEvent = EVENT_NONE;
        break;
      }

      case EVENT_STAIR_DOWN: {
        if (map.retreatFloor()) {
          applyFloorMapConfig(ctx);
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
        int who_to_add =
            map.getAutomatonWhoId();
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
        map.setTile(
            heroTileX,
            heroTileY,
            Map::TILE_GRASS
        );
        
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
            (map.getFloor() / 10) - 1;

        // 撃破済みボスのタイルが何らかの理由で残っていた場合
        if (map.getFloor() >= 10 &&
            map.getFloor() <= 50 &&
            map.getFloor() % 10 == 0 &&
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
            map.getFloor(),
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