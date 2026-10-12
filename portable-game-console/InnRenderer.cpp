#include "InnRenderer.h"

#include "InnMenu.h"
#include "Graphic.h"

void InnRenderer::drawLine(const InnMenu& menu, Graphic& graphic,
                           int drawY) const {
  int x = 60, y = 80 - drawY;
  graphic.lineSprite.fillRect(x, y, 200, 100, TFT_DARKGREY);
  graphic.lineSprite.drawRect(x, y, 200, 100, TFT_WHITE);

  graphic.lineSprite.setTextSize(2);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(x + 20, y + 20);
  graphic.lineSprite.print("一泊 10G です");

  graphic.lineSprite.setCursor(x + 40, y + 50);
  graphic.lineSprite.setTextColor(menu.cursor() == 0 ? TFT_YELLOW : TFT_WHITE);
  graphic.lineSprite.print("泊まる");

  graphic.lineSprite.setCursor(x + 40, y + 75);
  graphic.lineSprite.setTextColor(menu.cursor() == 1 ? TFT_YELLOW : TFT_WHITE);
  graphic.lineSprite.print("やめる");
}
