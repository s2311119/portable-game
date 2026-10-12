#include "MainMenuRenderer.h"

#include "MainMenu.h"
#include "Graphic.h"

void MainMenuRenderer::drawLine(const MainMenu& menu, Graphic& graphic,
                                int drawY) const {
  int x = 60, y = 40 - drawY;
  int w = 200, h = 190;
  graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
  graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
  graphic.lineSprite.setTextSize(2);
  for (int i = 0; i < menu.itemCount(); i++) {
    uint16_t color = (i == menu.selectedIndex()) ? TFT_YELLOW : TFT_WHITE;
    graphic.lineSprite.setTextColor(color);
    graphic.lineSprite.setCursor(x + 20, y + 20 + i * 28);
    graphic.lineSprite.print(menu.itemLabelAt(i));
  }
}
