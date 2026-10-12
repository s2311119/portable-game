#include "SettingsRenderer.h"

#include "SettingsMenu.h"
#include "Settings.h"
#include "Graphic.h"

void SettingsRenderer::drawLine(const SettingsMenu& menu,
                                const Settings& settings, Graphic& graphic,
                                int drawY) const {
  int x = 60, y = 40 - drawY;
  int w = 200, h = 160;
  graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
  graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);

  graphic.lineSprite.setTextSize(2);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(x + 20, y + 15);
  graphic.lineSprite.print("環境設定");
  graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

  auto drawItem = [&](int idx, String label, String value) {
    if (menu.cursor() == idx) graphic.lineSprite.setTextColor(TFT_YELLOW);
    else graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 55 + idx * 25);
    graphic.lineSprite.print(label);
    graphic.lineSprite.setCursor(x + 140, y + 55 + idx * 25);
    graphic.lineSprite.print(value);
  };

  String bgmValStr = String(settings.getBgmVolume());
  drawItem(0, "BGM", bgmValStr);

  String seValStr = String(settings.getSeVolume());
  drawItem(1, "SE", seValStr);

  String vibValStr = settings.isVibrationEnabled() ? "ON" : "OFF";
  drawItem(2, "振動", vibValStr);

  if (menu.cursor() == 3) graphic.lineSprite.setTextColor(TFT_YELLOW);
  else graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setCursor(x + 60, y + 130);
  graphic.lineSprite.print("戻る");
}
