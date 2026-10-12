#pragma once

#include <Arduino.h>
#include <vector>
#include "Graphic.h"
#include "Item.h"
#include "ItemCatalog.h"
#include "Skill.h"
#include "SkillCatalog.h"
#include "Controller.h"
#include "Inventory.h"
#include "Player.h"
#include "Party.h"
#include "AutomatonStorage.h"
#include "Map.h"
#include "WorldProgress.h"
#include "MUSIC.h"
#include "Sd.h"
#include "SaveManager.h"
#include "Settings.h"
#include "Vibration.h"
#include "MainMenu.h"
#include "MainMenuRenderer.h"
#include "ItemMenu.h"
#include "ItemRenderer.h"
#include "ItemUseSystem.h"
#include "SkillMenu.h"
#include "SkillRenderer.h"
#include "EquipmentMenu.h"
#include "EquipmentRenderer.h"
#include "EvolutionMenu.h"
#include "EvolutionRenderer.h"
#include "ShopMenu.h"
#include "ShopRenderer.h"
#include "InnMenu.h"
#include "InnRenderer.h"
#include "StorageMenu.h"
#include "StorageRenderer.h"
#include "SettingsMenu.h"
#include "SettingsRenderer.h"

class MenuFlow {
public:
  enum class MenuScreen {
    MAIN,
    EVOLUTION,
    SKILL,
    ITEM,
    STORAGE,
    SAVE_RESULT,
    EQUIPMENT,
    INN,
    SHOP,
    SETTINGS
  };
  bool saveSucceeded = false;

  void toggle() {
    active_ = !active_;
    screen_ = MenuScreen::MAIN;
    mainMenu.reset();
  }

  void openShop() {
    active_ = true;
    screen_ = MenuScreen::SHOP;
    shopMenu.open();
  }

  void openInn() {
    active_ = true;
    screen_ = MenuScreen::INN;
    innMenu.open();
  }

  void openStorage() {
    active_ = true;
    screen_ = MenuScreen::STORAGE;
    storageMenu.open();
  }

  bool isActive() const { return active_; }
  MenuScreen screen() const { return screen_; }

  void update(Controller &ctrl, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, Map& map, WorldProgress& progress, MUSIC& music, Sd& sd, SaveManager& saveMgr, Settings& settings, Vibration& vib);

  bool takeSettingsSaveRequest() {
    bool requested = settingsSaveRequested;
    settingsSaveRequested = false;
    return requested;
  }

  void draw(Graphic* g, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, const Settings& settings) {
    if (!active_) return;

    // ★ スプライト生成をやめて g->lineSprite を使う

    // --- 分割描画ループ ---
    for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {
      g->lineSprite.fillScreen(TFT_BLACK);
      int yOffset = -drawY;
      // 各要素の描画 (Y座標から drawY を引く)
      // ※ 以下、sp. を g->lineSprite. に置換

      if (screen_ == MenuScreen::MAIN) {
        mainMenuRenderer.drawLine(mainMenu, *g, drawY);

      } else if (screen_ == MenuScreen::EVOLUTION) {
        evolutionRenderer.drawLine(evolutionMenu, party, *g, drawY);

      } else if (screen_ == MenuScreen::SKILL) {
        skillRenderer.drawLine(skillMenu, inventory, hero, *g, drawY);

      } else if (screen_ == MenuScreen::ITEM) {
        itemRenderer.drawLine(itemMenu, inventory, hero, party, *g, drawY);

      } else if (screen_ == MenuScreen::STORAGE) {
        storageRenderer.drawLine(storageMenu, party, storage, *g, drawY);
      } else if (screen_ == MenuScreen::SAVE_RESULT) {
        // --- セーブ完了画面 ---
        int x = 60, y = 100 - drawY;
        int w = 200, h = 60;

        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);

        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 30, y + 20);
        if (saveSucceeded) {
          g->lineSprite.print("記録しました");
        }
        else {
          g->lineSprite.print("記録に失敗しました");
        }
      }else if (screen_ == MenuScreen::EQUIPMENT) {
        equipmentRenderer.drawLine(equipmentMenu, inventory, hero, *g, drawY);
      }else if (screen_ == MenuScreen::INN) {
        innRenderer.drawLine(innMenu, *g, drawY);
      }else if (screen_ == MenuScreen::SHOP) {
        shopRenderer.drawLine(shopMenu, inventory, *g, drawY);
      }else if (screen_ == MenuScreen::SETTINGS) {
        settingsRenderer.drawLine(settingsMenu, settings, *g, drawY);
      }

      // ★★★ 画面に転送 ★★★
      g->lineSprite.pushSprite(0, drawY);
    }

  }

private:
  MenuScreen screen_ = MenuScreen::MAIN;
  bool active_ = false;
  MainMenu mainMenu;
  MainMenuRenderer mainMenuRenderer;
  ItemMenu itemMenu;
  ItemRenderer itemRenderer;
  ItemUseSystem itemUseSystem;
  SkillMenu skillMenu;
  SkillRenderer skillRenderer;
  EquipmentMenu equipmentMenu;
  EquipmentRenderer equipmentRenderer;
  EvolutionMenu evolutionMenu;
  EvolutionRenderer evolutionRenderer;
  ShopMenu shopMenu;
  ShopRenderer shopRenderer;
  InnMenu innMenu;
  InnRenderer innRenderer;
  StorageMenu storageMenu;
  StorageRenderer storageRenderer;
  SettingsMenu settingsMenu;
  SettingsRenderer settingsRenderer;
  bool settingsSaveRequested = false;
};
