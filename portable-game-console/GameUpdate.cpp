#include "Game.h"

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

void Game::update() {
  Game* ctx = this;
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
          ctx->startBattleEncounter(spec, battleBgm);
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
    ctx->drawMenuFlow();
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
          ctx->applyFloorMapConfig();
        }
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::FLOOR_DOWN:
        if (ctx->floorManager.moveDown(ctx->session->progress())) {
          ctx->applyFloorMapConfig();
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
          ctx->startBattleEncounter(spec, battleBgm);
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
