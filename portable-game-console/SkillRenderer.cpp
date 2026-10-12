#include "SkillRenderer.h"

#include "SkillMenu.h"
#include "Inventory.h"
#include "Player.h"
#include "Graphic.h"
#include "SkillCatalog.h"
#include "ItemCatalog.h"

void SkillRenderer::drawLine(const SkillMenu& menu, const Inventory& inventory,
                             const Player& hero, Graphic& graphic,
                             int drawY) const {
  int x = 10, y = 40 - drawY;
  int w = 300, h = 190;
  graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
  graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
  graphic.lineSprite.setTextSize(2);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(x + 20, y + 15);
  graphic.lineSprite.print("旋律 (何を習得？)");
  graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

  int itemsPerPage = 3;
  int startIdx = 0;
  if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
  int endIdx = startIdx + itemsPerPage;
  if (endIdx > menu.skillCount()) endIdx = menu.skillCount();

  for (int i = startIdx; i < endIdx; i++) {
    Skill skill = SkillCatalog::get(menu.skillIdAt(i));
    int scrollId = 200 + skill.id;
    bool hasScroll = (inventory.countItem(scrollId) > 0);
    bool alreadyLearned = false;
    alreadyLearned = hero.state().skills().has(skill.id);
    uint16_t color = TFT_WHITE;
    if (i == menu.cursor()) color = TFT_YELLOW;
    else if (alreadyLearned) color = TFT_GREEN;
    else if (!hasScroll) color = TFT_LIGHTGREY;

    graphic.lineSprite.setTextColor(color);
    graphic.lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
    String statusStr = "";
    if (alreadyLearned) statusStr = "(済)";
    graphic.lineSprite.printf("%s%s (TP:%d)", skill.name, statusStr.c_str(), skill.tpCost);
  }

  graphic.lineSprite.drawLine(x, y + 140, x + w, y + 140, TFT_WHITE);
  int targetSkillId = menu.skillIdAt(menu.cursor());
  int reqScrollId = 200 + targetSkillId;
  int scrollCount = inventory.countItem(reqScrollId);
  graphic.lineSprite.setCursor(x + 20, y + 155);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  bool isLearned = false;
  isLearned = hero.state().skills().has(targetSkillId);
  if (isLearned) { graphic.lineSprite.setTextColor(TFT_GREEN); graphic.lineSprite.print("習得済みです"); }
  else if (scrollCount > 0) { graphic.lineSprite.setTextColor(TFT_CYAN); graphic.lineSprite.printf("習得可能！ (所持: %d)", scrollCount); }
  else { graphic.lineSprite.setTextColor(TFT_RED); graphic.lineSprite.setTextSize(1); graphic.lineSprite.setCursor(x + 20, y + 160); graphic.lineSprite.printf("必要: %s", ItemCatalog::get(reqScrollId).name); }
}
