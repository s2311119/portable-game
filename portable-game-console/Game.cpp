#include "Game.h"

Game* ctx = nullptr;

void Game::drawMenuFlow() {
  Game& context = *this;
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

void Game::startBattleEncounter(const EncounterSpec& spec,
                                const BattleBgmHooks& bgm) {
  Game& context = *this;
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

void Game::applyFloorMapConfig()
{
  Game* ctx = this;
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
