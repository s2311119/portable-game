#include "Game.h"

struct LoadGameOperation {
  Game* context;
  LoadGameResult result;
};

void loadGameAndRegenerateLegacy(void* rawOperation) {
  LoadGameOperation& operation = *static_cast<LoadGameOperation*>(rawOperation);
  operation.context->loadGameAndRegenerateLegacy(operation.result);
}

void Game::loadGameAndRegenerateLegacy(LoadGameResult& result) {
  Game* ctx = this;
  result = ctx->saveManager->loadGame(
      *ctx->sd, *ctx->session);

  const LoadGameResult& loadResult = result;
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

void Game::boot() {
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
  ctx = new Game();
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

  multicore_launch_core1(Game::core1Main);
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
            &loadOperation, ::loadGameAndRegenerateLegacy);
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
