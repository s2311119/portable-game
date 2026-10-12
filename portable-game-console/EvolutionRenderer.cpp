#include "EvolutionRenderer.h"

#include "EvolutionMenu.h"
#include "Party.h"
#include "Graphic.h"

void EvolutionRenderer::drawLine(const EvolutionMenu& menu, const Party& party,
                                 Graphic& graphic, int drawY) const {
  int x = 10, y = 40 - drawY;
  int w = 300, h = 190;
  graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
  graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
  graphic.lineSprite.setTextSize(2);

  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(x + 20, y + 15);
  graphic.lineSprite.print("進化 (誰を？)");
  graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

  int memberCount = party.memberCount();
  if (memberCount == 0) {
    graphic.lineSprite.setCursor(x + 20, y + 55);
    graphic.lineSprite.print("仲間がいません");
  } else {
    for (int i = 0; i < memberCount; i++) {
      if (i >= 5) break;
      const Automaton& ally = party.member(i);
      uint16_t color = (i == menu.cursor()) ? TFT_YELLOW : TFT_WHITE;
      graphic.lineSprite.setTextColor(color);
      graphic.lineSprite.setCursor(x + 20, y + 55 + i * 28);
      graphic.lineSprite.printf("%s Lv%d HP%d/%d", ally.state().name(), ally.state().progress().level(), ally.state().stats().getHp(), ally.state().stats().getMaxHp());
    }
  }
}
