// main.ino
#include <SPI.h>
#include <SdFat.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "bmp.h"
#include "Item.h"
#include "ItemCatalog.h"
#include "CharacterState.h"
#include "CharacterProgressionSystem.h"
#include "CombatCalculator.h"
#include "SkillEffectSystem.h"
#include "Automaton.h"
#include "AutomatonStorage.h"
#include "Party.h"
#include "Enemy.h"
#include "Player.h"
#include "Graphic.h"
#include "WorldRenderer.h"
#include "TitleRenderer.h"
#include "GameOverRenderer.h"
#include "LegacyGameTypes.h"
#include "Controller.h"
#include "EncounterManager.h"
#include "EncounterSpec.h"
#include "Exploration.h"
#include "FieldEventHandler.h"
#include "Vibration.h"
#include "Skill.h"
#include "SkillCatalog.h"
#include "Inventory.h"
#include "Map.h"
#include "WorldProgress.h"
#include "FloorManager.h"
#include "Settings.h"
#include "AudioManager.h"
#include "BgmCue.h"
#include "Storage.h"
#include "SaveManager.h"
#include "ImageManager.h"
#include "GameSession.h"
#include "Battle.h"
#include "BattleRenderer.h"
#include "MenuFlow.h"
#include "MainMenuRenderer.h"
#include "ItemRenderer.h"
#include "SkillRenderer.h"
#include "EquipmentRenderer.h"
#include "EvolutionRenderer.h"
#include "ShopRenderer.h"
#include "InnRenderer.h"
#include "StorageRenderer.h"
#include "SettingsRenderer.h"
#include "MemoryMonitor.h"
#include <utility>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"


#define LGFX_USE_SD 1
#define H_TRANSPARENT 0xFFFF
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





// main.ino


// main.ino の Item クラスの後に追加












// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------





// -------------------------
// AUDIO / SD (core1 に限定)
// -------------------------
bool audio_timer_callback(struct repeating_timer *t);


namespace field_input_adapter {
bool pressed(void* context, FieldDirection direction) {
  Controller& ctrl = *static_cast<Controller*>(context);
  switch (direction) {
    case FieldDirection::UP: return ctrl.pressed(ctrl.BTN_UP);
    case FieldDirection::DOWN: return ctrl.pressed(ctrl.BTN_DOWN);
    case FieldDirection::LEFT: return ctrl.pressed(ctrl.BTN_LEFT);
    case FieldDirection::RIGHT: return ctrl.pressed(ctrl.BTN_RIGHT);
  }
  return false;
}
} // namespace field_input_adapter

namespace field_random_adapter {
long drawMax(void*, long max) { return random(max); }
long drawRange(void*, long min, long max) { return random(min, max); }
} // namespace field_random_adapter

// Battleの意味イベントをBGM cueへ接続する。
namespace battle_bgm_adapter {
struct Context {
  AudioManager* music;
  Map* map;
};

void onEvent(void* rawContext, BattleBgmEvent event, BattleKind kind) {
  Context& context = *static_cast<Context*>(rawContext);
  switch (event) {
    case BattleBgmEvent::START:
      context.music->playBGM(
          kind == BattleKind::FINAL_BOSS ? BgmCue::BATTLE_FINAL :
          kind == BattleKind::NORMAL ? BgmCue::BATTLE_NORMAL : BgmCue::BATTLE_BOSS);
      break;
    case BattleBgmEvent::EARLY_FIELD_RETURN:
      context.music->playBGM(BgmCue::DUNGEON);
      break;
    case BattleBgmEvent::FINAL_FIELD_RETURN:
      if (context.map->isTown()) context.music->playBGM(BgmCue::TOWN);
      else context.music->playBGM(BgmCue::DUNGEON);
      break;
    case BattleBgmEvent::DEFEAT:
      context.music->playBGM(BgmCue::GAME_OVER);
      break;
  }
}
} // namespace battle_bgm_adapter

namespace menu_feedback_adapter {
struct Context {
  AudioManager* music;
  Vibration* vibration;
};

void playSE(void* rawContext, int seId) {
  static_cast<Context*>(rawContext)->music->playSE(seId);
}

void settingsChanged(void* rawContext, SettingsMenuChange change,
                     const Settings& settings) {
  Context& context = *static_cast<Context*>(rawContext);
  if (change == SettingsMenuChange::BGM_CHANGED) {
    context.music->bgmVolume = (float)settings.getBgmVolume() / 10.0f;
  } else if (change == SettingsMenuChange::SE_CHANGED) {
    context.music->setSeVolume(settings.getSeVolume());
    context.music->playSE(0);
  } else if (change == SettingsMenuChange::VIBRATION_CHANGED) {
    context.vibration->setEnabled(settings.isVibrationEnabled());
    if (settings.isVibrationEnabled()) context.vibration->trigger(100);
  }
}
} // namespace menu_feedback_adapter

namespace save_sync_adapter {
void begin(void* context) {
  static_cast<AudioManager*>(context)->beginStorageOperation();
}
void end(void* context) {
  static_cast<AudioManager*>(context)->endStorageOperation();
}
} // namespace save_sync_adapter

// ---------
// saveクラス
// ---------



// -------------------------
// BATTLE クラス
// -------------------------


struct GameContext {
  GameSession* session = nullptr;
  Graphic* graphic;
  WorldRenderer worldRenderer;
  TitleRenderer titleRenderer;
  GameOverRenderer gameOverRenderer;
  Controller* controller;
  AudioManager* music;
  Storage* sd;
  MenuFlow* menu;
  MainMenuRenderer mainMenuRenderer;
  ItemRenderer itemRenderer;
  SkillRenderer skillRenderer;
  EquipmentRenderer equipmentRenderer;
  EvolutionRenderer evolutionRenderer;
  ShopRenderer shopRenderer;
  InnRenderer innRenderer;
  StorageRenderer storageRenderer;
  SettingsRenderer settingsRenderer;
  EncounterManager* encounterManager;
  Exploration exploration;
  FieldEventHandler fieldEventHandler;
  FloorManager floorManager;
  GameState state;
  FieldEvent currentEvent = {FieldEventType::NONE, 0, 0};
  SaveManager* saveManager;
  Battle* battle;
  BattleRenderer battleRenderer;
  bool requestRegenerate;
  Vibration* vibration;
  ImageManager* imgMgr;
  MemoryMonitor memMonitor;
  Settings settings;
};

struct LoadGameOperation {
  GameContext* context;
  LoadGameResult result;
};

void loadGameAndRegenerateLegacy(void* rawOperation) {
  LoadGameOperation& operation = *static_cast<LoadGameOperation*>(rawOperation);
  GameContext* ctx = operation.context;
  operation.result = ctx->saveManager->loadGame(
      *ctx->sd, *ctx->session);

  const LoadGameResult& loadResult = operation.result;
  if (loadResult.success && loadResult.requiresMapRegeneration) {
    Map::MapType finalType = loadResult.legacyMapType;
    WorldProgress& progress = ctx->session->progress();

    // ボス階では撃破状態を優先
    if (progress.floor() >= 10 &&
        progress.floor() <= 50 &&
        progress.floor() % 10 == 0)
    {
      int bossIndex = (progress.floor() / 10) - 1;
      if (bossIndex >= 0 && bossIndex < 5) {
        if (progress.bosses().isDefeated(bossIndex)) {
          finalType = Map::TYPE_TOWN;
        } else {
          finalType = Map::TYPE_BOSS_ROOM;
        }
      }
    }

    ctx->session->map().regenerate(
        20, 20, finalType, progress.floor());
    ctx->session->player().reset(ctx->session->map());
  }
}

GameContext* ctx = nullptr;

void drawMenuFlow(GameContext& context) {
  const MenuFlow& menu = *context.menu;
  if (!menu.isActive()) return;

  Graphic& graphic = *context.graphic;
  const GameSession& session = *context.session;
  for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {
    graphic.lineSprite.fillScreen(TFT_BLACK);

    if (menu.screen() == MenuFlow::MenuScreen::MAIN) {
      context.mainMenuRenderer.drawLine(menu.main(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::EVOLUTION) {
      context.evolutionRenderer.drawLine(menu.evolution(), session.party(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::SKILL) {
      context.skillRenderer.drawLine(menu.skill(), session.inventory(), session.player(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::ITEM) {
      context.itemRenderer.drawLine(menu.item(), session.inventory(), session.player(), session.party(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::STORAGE) {
      context.storageRenderer.drawLine(menu.storageScreen(), session.party(), session.storage(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::SAVE_RESULT) {
      int x = 60, y = 100 - drawY;
      int w = 200, h = 60;
      graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
      graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
      graphic.lineSprite.setTextSize(2);
      graphic.lineSprite.setTextColor(TFT_WHITE);
      graphic.lineSprite.setCursor(x + 30, y + 20);
      if (menu.saveResultSucceeded()) graphic.lineSprite.print("記録しました");
      else graphic.lineSprite.print("記録に失敗しました");
    } else if (menu.screen() == MenuFlow::MenuScreen::EQUIPMENT) {
      context.equipmentRenderer.drawLine(menu.equipment(), session.inventory(), session.player(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::INN) {
      context.innRenderer.drawLine(menu.inn(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::SHOP) {
      context.shopRenderer.drawLine(menu.shop(), session.inventory(), graphic, drawY);
    } else if (menu.screen() == MenuFlow::MenuScreen::SETTINGS) {
      context.settingsRenderer.drawLine(menu.settingsScreen(), context.settings, graphic, drawY);
    }

    graphic.lineSprite.pushSprite(0, drawY);
  }
}

void startBattleEncounter(GameContext& context, const EncounterSpec& spec,
                          const BattleBgmHooks& bgm) {
  Battle& battle = *context.battle;
  if (battle.inBattle) return;

  Party& party = context.session->party();
  battle.beginStart(context.session->player(), party, spec);

  BattleImageRequest request{};
  if (spec.kind != BattleKind::NORMAL) {
    request.enemyCount = 1;
    request.enemyIds[0] = spec.enemies[0].enemyId;
  } else {
    request.enemyCount = static_cast<int>(battle.activeEnemies.size());
    if (request.enemyCount > 4) request.enemyCount = 4;
    for (int i = 0; i < request.enemyCount; ++i) {
      request.enemyIds[i] = battle.activeEnemies[i].who;
    }
  }
  request.playerCharacterId = 38;
  request.allyCount = static_cast<int>(party.memberCount());
  if (request.allyCount > 3) request.allyCount = 3;
  for (int i = 0; i < request.allyCount; ++i) {
    int id = party.member(i).state().id();
    request.allyCharacterIds[i] = id == 99 ? 38 : id;
  }

  context.imgMgr->prepareEnemyImages(request);
  bgm.emit(BattleBgmEvent::START, spec.kind);
  context.imgMgr->preparePartyImages(request);
  battle.finishStart();
}

void applyFloorMapConfig(GameContext* ctx);

void applyFloorMapConfig(GameContext* ctx)
{
  FloorMapConfig config =
      ctx->floorManager.getConfig(ctx->session->progress());

  ctx->music->playBGM(
      config.type == Map::TYPE_TOWN ? BgmCue::TOWN : BgmCue::DUNGEON
  );

  ctx->worldRenderer.setFloor(
      ctx->session->progress().floor()
  );

  ctx->session->map().regenerate(
      config.width,
      config.height,
      config.type,
      ctx->session->progress().floor()
  );

  ctx->session->player().reset(ctx->session->map());
}

// タイマー割り込み関数

bool audio_timer_callback(struct repeating_timer *t) {
  AudioManager* music = ctx->music;
  
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
  if (music->seType != AudioManager::OFF) {
    int16_t seWave = 0;

    // 音量の減衰
    music->seVol -= music->seVolDecay;
    if (music->seVol <= 0) {
      music->seType = AudioManager::OFF; // 音量0で終了
      music->seVol = 0;
    } else {
      // 周波数の変化 (スライド)
      music->seFreq += music->seFreqSlide;
      
      // 波形生成
      if (music->seType == AudioManager::SQUARE) {
        // 矩形波: 位相を進めて High/Low を切り替え
        // 16000Hz動作なので、位相は Freq / 16000 ずつ進む
        music->sePhase += music->seFreq / 16000.0f;
        if (music->sePhase >= 1.0f) music->sePhase -= 1.0f;
        
        seWave = (music->sePhase < 0.5f) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      } 
      else if (music->seType == AudioManager::NOISE) {
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
    ctx->music->performPendingSwitch(*ctx->sd);
    ctx->music->refile(*ctx->sd);

    tight_loop_contents();
  }
}

// main.ino (setup 関数)

void setup() {
  Serial.begin(115200);

  Storage::initializeSynchronization();
  
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
  ctx->music = new AudioManager();
  ctx->sd = new Storage();
  ctx->imgMgr = new ImageManager(*ctx->sd);
  // Graphic, SD の初期化
  ctx->graphic->init();
  ctx->sd->init();
  ctx->music->initializeTracks(*ctx->sd);
  // SaveManager 生成
  ctx->saveManager = new SaveManager();
  ctx->vibration = new Vibration();
  ctx->vibration->init();

  ctx->saveManager->loadSettings(*ctx->sd, ctx->settings);
  ctx->music->bgmVolume = (float)ctx->settings.getBgmVolume() / 10.0f;
  ctx->music->setSeVolume(ctx->settings.getSeVolume());
  ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());

  multicore_launch_core1(core1_main);
  delay(200);

  ctx->music->playBGM(BgmCue::TITLE);
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
        ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
      } else {
        // ロード用の仮オブジェクト生成
        ctx->session = new GameSession();

        LoadGameOperation loadOperation{ctx, {false, false, Map::TYPE_DUNGEON}};
        ctx->music->runWhileStoragePaused(
            &loadOperation, loadGameAndRegenerateLegacy);
        const LoadGameResult& loadResult = loadOperation.result;

        if (loadResult.success) {
          loadSuccess = true;
          ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
        } else {
          ctx->music->playSE(1);
          startNewGame = true; // ロード失敗時は新規
          ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
        }
      }
    }

    // ★★★ ラインスプライトによる分割描画 ★★★
    for (int drawY = 0; drawY < Graphic::SCREEN_HEIGHT; drawY += Graphic::BLOCK_HEIGHT) {
      ctx->graphic->lineSprite.fillScreen(TFT_BLACK);
      ctx->titleRenderer.drawLine(*ctx->graphic, titleCursor, hasSave, drawY);

      // 画面に転送
      ctx->graphic->lineSprite.pushSprite(0, drawY);
    }
  }

  // 3. ゲーム開始準備
  if (startNewGame) {
    // 新規作成
    if (ctx->session) delete ctx->session;
    ctx->session = new GameSession();

    Map::ObjectPos startPos =
        ctx->session->map().getPlayerStartPos();

    ctx->session->player().X =
        startPos.x * Map::TILE_SIZE;

    ctx->session->player().Y =
        startPos.y * Map::TILE_SIZE;

    // 初期アイテム
    ctx->session->inventory().addItem(ItemCatalog::get(100)); // リペアキット
    
    Automaton* firstAlly = new Automaton(1, 39);
    ctx->session->party().addMember(std::move(*firstAlly));
    delete firstAlly;
  }

  // 共通初期化
  ctx->menu = new MenuFlow();
  ctx->encounterManager = new EncounterManager();
  ctx->battle = new Battle();

  // WorldRendererへ階層通知
  ctx->worldRenderer.setFloor(ctx->session->progress().floor());

  // 初回のフィールド描画
  int camX = ctx->session->player().X - Graphic::SCREEN_WIDTH / 2;
  int camY = ctx->session->player().Y - Graphic::SCREEN_HEIGHT / 2;
  ctx->worldRenderer.drawFullBackground(*ctx->graphic, ctx->session->map(), ctx->session->player(), camX, camY);
  if (ctx->session->map().isTown()) {
    // 集落なら bgm_0.raw
    ctx->music->playBGM(BgmCue::TOWN);
  } else {
    // ダンジョンなら bgm_1.raw
    ctx->music->playBGM(BgmCue::DUNGEON);
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
    ctx->session->map().regenerate(20, 20, Map::TYPE_DUNGEON, ctx->session->progress().floor());
    ctx->session->player().reset(ctx->session->map());

    ctx->worldRenderer.setFloor(ctx->session->progress().floor());

    int cameraX = ctx->session->player().X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = ctx->session->player().Y - Graphic::SCREEN_HEIGHT / 2;
    ctx->worldRenderer.invalidateAndRedraw(*ctx->graphic, ctx->session->map(), ctx->session->player(), cameraX, cameraY);
    interrupts();
  }
  static GameState prevState = STATE_GAME;
  static int gameOverCursor = 0;
  auto& g = *ctx->graphic;
  auto& worldRenderer = ctx->worldRenderer;
  auto& map = ctx->session->map();
  auto& hero = ctx->session->player();
  auto& ctrl = *ctx->controller;
  auto& menu = *ctx->menu;
  auto& encounterManager = *ctx->encounterManager;
  auto& inventory = ctx->session->inventory();
  auto& party = ctx->session->party();
  auto& storage = ctx->session->storage();
  auto& state = ctx->state;
  auto& music = *ctx->music;
  auto& sd = *ctx->sd;
  auto& currentEvent = ctx->currentEvent;
  auto& save = *ctx->saveManager;
  auto& vib = *ctx->vibration;
  auto& imgmgr = *ctx->imgMgr;
  auto& settings = ctx->settings;
  battle_bgm_adapter::Context battleBgmContext{&music, &map};
  const BattleBgmHooks battleBgm{&battleBgmContext, battle_bgm_adapter::onEvent};
  vib.update();
  if (state == STATE_GAME) {
    hero.oldX = hero.X;
    hero.oldY = hero.Y;

    int prevX = hero.X, prevY = hero.Y;
    uint32_t nowMs = millis();
    ExplorationInput fieldInput = {&ctrl, field_input_adapter::pressed};
    ctx->exploration.updateMovement(hero, map, nowMs, fieldInput);

    bool actionPressed = ctrl.pressedDebounced(ctrl.BTN_A);
    ExplorationResult fieldResult =
        ctx->exploration.completeStep(hero, map, prevX, prevY, actionPressed);
    bool eventOccurred = fieldResult.hasEvent;
    if (eventOccurred) {
      state = STATE_EVENT;
      currentEvent = fieldResult.event;
    }
    // 2. 移動が発生したかチェック
    bool moved = fieldResult.moved;
    // 3. 移動していて、メニューが開いていない場合のみイベントとエンカウントをチェック
    if (moved && !menu.isActive()) {
      // 5. イベントが発生しなかった場合のみ、エンカウントをチェック
      EncounterRandom encounterRandom = {nullptr,
                                         field_random_adapter::drawMax,
                                         field_random_adapter::drawRange};
      if (!eventOccurred && encounterManager.onStep(map, encounterRandom)) {
        vib.trigger(300);
        state = STATE_BATTLE;
        if (!ctx->battle->inBattle) {
          EncounterSpec spec = encounterManager.createEncounter(
              ctx->session->progress().floor(), map, encounterRandom);
          startBattleEncounter(*ctx, spec, battleBgm);
        }
      }
    }

    // 置換箇所（STATE_GAME 内）
    int cameraX = hero.X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = hero.Y - Graphic::SCREEN_HEIGHT / 2;
    // map が再生成されたときは drawFullBackground を呼んでいるので通常は smartRender だけでOK
    worldRenderer.smartRender(g, map, hero, cameraX, cameraY);


    if (ctrl.pressed(ctrl.BTN_X)) {
      menu.toggle();
      if (menu.isActive()) state = STATE_MENU;
    }
  }

  else if (state == STATE_MENU) {
    //menu.update(ctrl, inventory, hero, party);
    menu_feedback_adapter::Context feedbackContext{&music, &vib};
    MenuFeedbackHooks feedback{&feedbackContext,
                               menu_feedback_adapter::playSE,
                               menu_feedback_adapter::settingsChanged};
    MenuFlowResult menuResult = menu.update(
        ctrl, inventory, hero, party, storage,
        ctx->session->progress(), settings, feedback);
    const SaveSyncHooks saveSync{&music, save_sync_adapter::begin,
                                 save_sync_adapter::end};
    if (menuResult == MenuFlowResult::REQUEST_SAVE) {
      menu.finishSave(save.saveGame(sd, *ctx->session, saveSync));
    }
    if (menuResult == MenuFlowResult::SAVE_SETTINGS) {
      save.saveSettings(sd, settings, saveSync);
    }
    //g.screenSprite.fillScreen(TFT_BLACK);
    drawMenuFlow(*ctx);
    //g.screenSprite.pushSprite(0,0);

    if (menuResult == MenuFlowResult::CLOSE_MENU) {
      state = STATE_GAME; // ゲーム画面に戻る
      //もしうまくいかなかったらここを
      ctx->requestRegenerate = false;
    }
  }

  else if (state == STATE_BATTLE) {
    auto &battle = *ctx->battle;
    battle.update(ctrl, music, hero, inventory, vib, battleBgm);
    if (const std::optional<BattleResult> result = battle.takeResult()) {
      if (result->outcome == BattleOutcome::VICTORY && result->kind != BattleKind::NORMAL) {
        WorldProgress& progress = ctx->session->progress();
        int bossIndex = (progress.floor() / 10) - 1;
        if (bossIndex >= 0 && bossIndex < 5) {
          progress.markBossDefeated(bossIndex);
        }

        map.regenerate(20, 20, Map::TYPE_TOWN, progress.floor());
        hero.reset(map);

        int cx = hero.X - Graphic::SCREEN_WIDTH/2;
        int cy = hero.Y - Graphic::SCREEN_HEIGHT/2;
        worldRenderer.invalidateAndRedraw(g, map, hero, cx, cy);

        music.playBGM(BgmCue::TOWN);
      }
      state = result->outcome == BattleOutcome::DEFEAT ? STATE_GAME_OVER : STATE_GAME;
    }
    ctx->battleRenderer.draw(battle, hero, party, imgmgr, g);
    //g.screenSprite.pushSprite(0, 0);
  }

  else if (state == STATE_EVENT) {
    FieldRandom fieldRandom = {nullptr,
                               field_random_adapter::drawMax,
                               field_random_adapter::drawRange};
    FieldEventResult eventResult = ctx->fieldEventHandler.handle(
        currentEvent, map, inventory, party, storage,
        ctx->session->progress().floor(), fieldRandom);

    switch (eventResult.action) {
      case FieldEventAction::RETURN_TO_FIELD:
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::FLOOR_UP:
        if (ctx->floorManager.moveUp(ctx->session->progress())) {
          applyFloorMapConfig(ctx);
        }
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::FLOOR_DOWN:
        if (ctx->floorManager.moveDown(ctx->session->progress())) {
          applyFloorMapConfig(ctx);
        }
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_INN:
        ctx->menu->openInn();
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_STORAGE:
        menu.openStorage();
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_SHOP:
        ctx->menu->openShop();
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::START_BOSS_BATTLE: {
        int bossIndex =
            (ctx->session->progress().floor() / 10) - 1;

        if (ctx->session->progress().floor() >= 10 &&
            ctx->session->progress().floor() <= 50 &&
            ctx->session->progress().floor() % 10 == 0 &&
            bossIndex >= 0 &&
            bossIndex < 5 &&
            ctx->session->progress().bosses().isDefeated(bossIndex))
        {
          map.regenerate(
              20,
              20,
              Map::TYPE_TOWN,
              ctx->session->progress().floor()
          );
          hero.reset(map);

          music.playBGM(BgmCue::TOWN);

          state = STATE_GAME;
          currentEvent.type = FieldEventType::NONE;
          break;
        }

        vib.trigger(500);

        state = STATE_BATTLE;

        if (!ctx->battle->inBattle) {
          EncounterRandom encounterRandom = {nullptr,
                                             field_random_adapter::drawMax,
                                             field_random_adapter::drawRange};
          EncounterSpec spec = encounterManager.createBossEncounter(
              ctx->session->progress().floor(), map, encounterRandom);
          startBattleEncounter(*ctx, spec, battleBgm);
        }

        currentEvent.type = FieldEventType::NONE;
        break;
      }

      case FieldEventAction::NONE:
      default:
        state = STATE_GAME;
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
      ctx->gameOverRenderer.drawLine(*ctx->graphic, drawY);

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
      worldRenderer.invalidateAndRedraw(g, map, hero, cameraX, cameraY);
    } else {
      // メニュー／戦闘へ入るときは「画面を上書きする」ので
      // prevHero を無効化して、restore が誤動作しないようにする
      worldRenderer.invalidatePreviousHero();
    }
    prevState = state;
  }
}