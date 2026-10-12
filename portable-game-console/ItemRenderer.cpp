#include "ItemRenderer.h"

#include "ItemMenu.h"
#include "Inventory.h"
#include "Player.h"
#include "Party.h"
#include "Graphic.h"

void ItemRenderer::drawLine(const ItemMenu& menu, const Inventory& inventory,
                            const Player& hero, const Party& party,
                            Graphic& graphic, int drawY) const {
  if (menu.state() == ItemMenu::State::ITEM_LIST) {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.print("道具 (一覧)");
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    int itemCount = inventory.items.size();
    if (itemCount == 0) {
      graphic.lineSprite.setCursor(x + 20, y + 55); graphic.lineSprite.print("道具を持っていません");
    } else {
      int itemsPerPage = 5;
      int startIdx = 0;
      if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
      int endIdx = startIdx + itemsPerPage;
      if (endIdx > itemCount) endIdx = itemCount;

      for (int i = startIdx; i < endIdx; i++) {
        int id = inventory.items[i].item.id;
        bool isUsable = (id >= 100 && id < 200);
        uint16_t color = TFT_WHITE;
        if (i == menu.cursor()) color = TFT_YELLOW;
        else if (!isUsable) color = TFT_LIGHTGREY;

        graphic.lineSprite.setTextColor(color);
        graphic.lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
        if (id >= 200 && id < 300) {
          // %sの巻物 と書くことで、画面上だけ結合される
          graphic.lineSprite.printf("%sの巻物 x%d", inventory.items[i].item.name, inventory.items[i].quantity);
        } else {
          // 通常アイテム
          graphic.lineSprite.printf("%s x%d", inventory.items[i].item.name, inventory.items[i].quantity);
        }
      }
    }
  } else {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.print("誰に使いますか？");
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    uint16_t color = (menu.cursor() == 0) ? TFT_YELLOW : TFT_WHITE;
    graphic.lineSprite.setTextColor(color);
    graphic.lineSprite.setCursor(x + 20, y + 55);
    graphic.lineSprite.printf("調律師 HP:%d/%d", hero.state().stats().getHp(), hero.state().stats().getMaxHp());

    for (int i = 0; i < party.memberCount(); i++) {
      color = (menu.cursor() == i + 1) ? TFT_YELLOW : TFT_WHITE;
      graphic.lineSprite.setTextColor(color);
      graphic.lineSprite.setCursor(x + 20, y + 55 + (i + 1) * 28);
      const auto* s = &party.member(i).state();
      graphic.lineSprite.printf("%s HP:%d/%d", s->name(), s->stats().getHp(), s->stats().getMaxHp());
    }
  }
}
