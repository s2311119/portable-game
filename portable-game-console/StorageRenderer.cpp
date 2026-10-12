#include "StorageRenderer.h"

#include "StorageMenu.h"
#include "Party.h"
#include "AutomatonStorage.h"
#include "Graphic.h"

void StorageRenderer::drawLine(const StorageMenu& menu, const Party& party,
                               const AutomatonStorage& storage,
                               Graphic& graphic, int drawY) const {
  graphic.lineSprite.fillScreen(TFT_BLACK);
  graphic.lineSprite.setTextColor(TFT_WHITE);
  graphic.lineSprite.setTextSize(2);
  graphic.lineSprite.setCursor(10, 10 - drawY);
  graphic.lineSprite.print("=== 預かり所 ===");

  if (menu.state() == StorageMenu::State::CHOICE) {
    graphic.lineSprite.setCursor(40, 50 - drawY);
    graphic.lineSprite.setTextColor(menu.cursor() == 0 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.print("仲間を引き出す");
    graphic.lineSprite.setCursor(40, 80 - drawY);
    graphic.lineSprite.setTextColor(menu.cursor() == 1 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.print("仲間を預ける");
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(40, 130 - drawY);
    graphic.lineSprite.printf("パーティー: %d/%d人", party.memberCount(), Party::MAX_MEMBERS);
    graphic.lineSprite.setCursor(40, 150 - drawY);
    graphic.lineSprite.printf("預かり所: %d人", storage.memberCount());
  }
  else if (menu.state() == StorageMenu::State::WITHDRAW) {
    graphic.lineSprite.setCursor(10, 35 - drawY);
    graphic.lineSprite.print("誰を引き出しますか？");
    if (party.memberCount() >= Party::MAX_MEMBERS) {
      graphic.lineSprite.setTextColor(TFT_RED);
      graphic.lineSprite.print(" (満員)");
    }
    int count = storage.memberCount();
    if (count == 0) {
      graphic.lineSprite.setCursor(30, 60 - drawY);
      graphic.lineSprite.setTextColor(TFT_LIGHTGREY);
      graphic.lineSprite.print("誰もいません");
    }

    int itemsPerPage = 6;
    int startIdx = 0;
    if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
    int endIdx = startIdx + itemsPerPage;
    if (endIdx > count) endIdx = count;

    for (int i = startIdx; i < endIdx; i++) {
      graphic.lineSprite.setCursor(30, 60 + (i - startIdx) * 25 - drawY);
      graphic.lineSprite.setTextColor(menu.cursor() == i ? TFT_YELLOW : TFT_WHITE);
      graphic.lineSprite.printf("%s Lv%d",
                                storage.member(i).state().name(),
                                storage.member(i).state().progress().level());
    }
  }
  else if (menu.state() == StorageMenu::State::DEPOSIT) {
    graphic.lineSprite.setCursor(10, 35 - drawY);
    graphic.lineSprite.print("誰を預けますか？");
    int count = party.memberCount();
    if (count == 0) {
      graphic.lineSprite.setCursor(30, 60 - drawY);
      graphic.lineSprite.print("誰もいません");
    }
    for (int i = 0; i < count; i++) {
      graphic.lineSprite.setCursor(30, 60 + i * 25 - drawY);
      graphic.lineSprite.setTextColor(menu.cursor() == i ? TFT_YELLOW : TFT_WHITE);
      graphic.lineSprite.printf("%s Lv%d",
                                party.member(i).state().name(),
                                party.member(i).state().progress().level());
    }
  }
}
