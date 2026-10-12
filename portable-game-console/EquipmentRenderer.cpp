#include "EquipmentRenderer.h"

#include "EquipmentMenu.h"
#include "Inventory.h"
#include "Player.h"
#include "Graphic.h"
#include "ItemCatalog.h"

void EquipmentRenderer::drawLine(const EquipmentMenu& menu,
                                 const Inventory& inventory,
                                 const Player& player, Graphic& graphic,
                                 int drawY) const {
  if (menu.state() == EquipmentMenu::State::SLOT) {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);

    const CharacterState& s = player.state();
    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.printf("装備 (%s)", s.name());
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    String wName = (s.equipmentState().weaponId() == -1) ? "なし" : ItemCatalog::get(s.equipmentState().weaponId()).name;
    String aName = (s.equipmentState().armorId() == -1) ? "なし" : ItemCatalog::get(s.equipmentState().armorId()).name;
    String acName = (s.equipmentState().accessoryId() == -1) ? "なし" : ItemCatalog::get(s.equipmentState().accessoryId()).name;

    graphic.lineSprite.setTextColor(menu.cursor() == 0 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 60); graphic.lineSprite.printf("武器: %s", wName.c_str());

    graphic.lineSprite.setTextColor(menu.cursor() == 1 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 90); graphic.lineSprite.printf("防具: %s", aName.c_str());

    graphic.lineSprite.setTextColor(menu.cursor() == 2 ? TFT_YELLOW : TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 120); graphic.lineSprite.printf("装飾: %s", acName.c_str());

    graphic.lineSprite.setTextColor(TFT_CYAN);
    graphic.lineSprite.setCursor(x + 20, y + 160);
    graphic.lineSprite.printf("ATK:%d DEF:%d", s.attack(), s.defense());
  } else {
    int x = 10, y = 40 - drawY;
    int w = 300, h = 190;
    graphic.lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
    graphic.lineSprite.drawRect(x, y, w, h, TFT_WHITE);
    graphic.lineSprite.setTextSize(2);
    graphic.lineSprite.setTextColor(TFT_WHITE);
    graphic.lineSprite.setCursor(x + 20, y + 15);
    graphic.lineSprite.print("装備するアイテム");
    graphic.lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

    std::vector<int> equippableIndices = menu.candidateIndices(inventory);
    if (equippableIndices.empty()) {
      graphic.lineSprite.setCursor(x + 20, y + 55); graphic.lineSprite.print("装備できるものがありません");
    } else {
      int itemsPerPage = 5;
      int startIdx = 0;
      if (menu.cursor() >= itemsPerPage) startIdx = menu.cursor() - (itemsPerPage - 1);
      int endIdx = min(startIdx + itemsPerPage, (int)equippableIndices.size());

      for (int i = startIdx; i < endIdx; i++) {
        int idx = equippableIndices[i];
        const Item& it = inventory.items[idx].item;

        graphic.lineSprite.setTextColor(menu.cursor() == i ? TFT_YELLOW : TFT_WHITE);
        graphic.lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);

        String valStr = "";
        if (it.type == Item::ITEM_WEAPON) valStr = "ATK+" + String(it.value);
        else if (it.type == Item::ITEM_ARMOR) valStr = "DEF+" + String(it.value);

        graphic.lineSprite.printf("%s (%s)", it.name, valStr.c_str());
      }
    }
  }
}
