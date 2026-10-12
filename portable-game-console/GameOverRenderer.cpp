#include "GameOverRenderer.h"

#include "Graphic.h"
#include <LGFX_TFT_eSPI.hpp>

void GameOverRenderer::drawLine(Graphic& graphic, int drawY) const {
  // 左上(Top-Left)を基準点にする。
  graphic.lineSprite.setTextDatum(TL_DATUM);

  graphic.lineSprite.setTextColor(TFT_RED);
  graphic.lineSprite.setTextSize(3);
  graphic.lineSprite.drawString("GAME OVER", 60, 80 - drawY);

  graphic.lineSprite.setTextSize(2);
  graphic.lineSprite.setTextColor(TFT_YELLOW);
  graphic.lineSprite.drawString("タイトルへ", 80, 150 - drawY);
}
