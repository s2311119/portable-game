#pragma once

#include <Arduino.h>
#include <vector>
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
#include "ItemMenu.h"
#include "ItemUseSystem.h"
#include "SkillMenu.h"
#include "EquipmentMenu.h"
#include "EvolutionMenu.h"
#include "ShopMenu.h"
#include "InnMenu.h"
#include "StorageMenu.h"
#include "SettingsMenu.h"

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
  bool saveResultSucceeded() const { return saveSucceeded; }
  const MainMenu& main() const { return mainMenu; }
  const ItemMenu& item() const { return itemMenu; }
  const SkillMenu& skill() const { return skillMenu; }
  const EquipmentMenu& equipment() const { return equipmentMenu; }
  const EvolutionMenu& evolution() const { return evolutionMenu; }
  const ShopMenu& shop() const { return shopMenu; }
  const InnMenu& inn() const { return innMenu; }
  const StorageMenu& storageScreen() const { return storageMenu; }
  const SettingsMenu& settingsScreen() const { return settingsMenu; }

  void update(Controller &ctrl, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, Map& map, WorldProgress& progress, MUSIC& music, Sd& sd, SaveManager& saveMgr, Settings& settings, Vibration& vib);

  bool takeSettingsSaveRequest() {
    bool requested = settingsSaveRequested;
    settingsSaveRequested = false;
    return requested;
  }

private:
  MenuScreen screen_ = MenuScreen::MAIN;
  bool active_ = false;
  bool saveSucceeded = false;
  MainMenu mainMenu;
  ItemMenu itemMenu;
  ItemUseSystem itemUseSystem;
  SkillMenu skillMenu;
  EquipmentMenu equipmentMenu;
  EvolutionMenu evolutionMenu;
  ShopMenu shopMenu;
  InnMenu innMenu;
  StorageMenu storageMenu;
  SettingsMenu settingsMenu;
  bool settingsSaveRequested = false;
};
