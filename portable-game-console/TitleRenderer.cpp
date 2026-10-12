#include "TitleRenderer.h"

#include "Graphic.h"
#include <LGFX_TFT_eSPI.hpp>

void TitleRenderer::drawLine(Graphic& graphic, int titleCursor,
                             bool hasSave, int drawY) const {
  // タイトルロゴ
  graphic.lineSprite.setTextSize(3);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(60, 80 - drawY);
  graphic.lineSprite.print("RPG GAME");

  // メニュー項目
  graphic.lineSprite.setTextSize(2);

  // 「はじめから」
  graphic.lineSprite.setTextColor(titleCursor == 0 ? TFT_YELLOW : TFT_WHITE);
  graphic.lineSprite.setCursor(100, 150 - drawY);
  graphic.lineSprite.print("はじめから");

  // 「つづきから」
  if (hasSave) graphic.lineSprite.setTextColor(titleCursor == 1 ? TFT_YELLOW : TFT_WHITE);
  else graphic.lineSprite.setTextColor(TFT_DARKGREY);
  graphic.lineSprite.setCursor(100, 180 - drawY);
  graphic.lineSprite.print("つづきから");
}
