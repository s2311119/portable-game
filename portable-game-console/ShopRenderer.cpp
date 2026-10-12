#include "ShopRenderer.h"

#include "ShopMenu.h"
#include "Inventory.h"
#include "Graphic.h"
#include "ItemCatalog.h"

void ShopRenderer::drawLine(const ShopMenu& menu, const Inventory& inventory,
                            Graphic& graphic, int drawY) const {
  if (menu.state() == ShopMenu::State::CHOICE) {
    int x = 60, y = 80 - drawY;
    graphic.lineSprite.fillRect(x, y, 200, 120, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, 200, 120, TFT_WHITE);

    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 10);
    graphic.lineSprite.print("いらっしゃい");

    graphic.lineSprite.setCursor(x + 40, y + 40);
    graphic.lineSprite.setTextColor(menu.cursor() == 0 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.print("買う");

    graphic.lineSprite.setCursor(x + 40, y + 65);
    graphic.lineSprite.setTextColor(menu.cursor() == 1 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.print("売る");

    graphic.lineSprite.setCursor(x + 40, y + 90);
    graphic.lineSprite.setTextColor(menu.cursor() == 2 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.print("出る");
  } else if (menu.state() == ShopMenu::State::BUY) {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);

    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.printf("買う (所持金:%dG)", inventory.money);
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    const int shopItems[] = {100, 101, 103, 104, 300, 301, 400, 401, 500};
    int itemCount = menu.stockCount();

    int itemsPerPage = 5;
    int startIdx = 0;
    if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
    int endIdx = min(startIdx + itemsPerPage, itemCount);

    for (int i = startIdx; i < endIdx; i++) {
      int itemId = menu.stockItemIdAt(i);
      Item it = ItemCatalog::get(itemId);

      graphic.lineSprite.setTextColor(menu.cursor() == i ? TFT_YELLOW : TFT_WHITE);
      graphic.lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
      graphic.lineSprite.printf("%s (%dG)", it.name, it.price);
    }
  } else if (menu.state() == ShopMenu::State::SELL) {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);

    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.printf("売る (所持金:%dG)", inventory.money);
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    int itemCount = inventory.items.size();
    if (itemCount == 0) {
      graphic.lineSprite.setCursor(x + 20, y + 55);
      graphic.lineSprite.print("売れるものがありません");
    } else {
      int itemsPerPage = 5;
      int startIdx = 0;
      if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
      int endIdx = min(startIdx + itemsPerPage, itemCount);

      for (int i = startIdx; i < endIdx; i++) {
        const Item& it = inventory.items[i].item;
        int sellPrice = it.price / 2;

        graphic.lineSprite.setTextColor(menu.cursor() == i ? TFT_YELLOW : TFT_WHITE);
        graphic.lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
        graphic.lineSprite.printf("%s x%d (%dG)", it.name, inventory.items[i].quantity, sellPrice);
      }
    }
  }
}
