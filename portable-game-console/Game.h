#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "Item.h"
#include "ItemCatalog.h"
#include "CharacterState.h"
#include "CharacterProgressionSystem.h"
#include "CombatCalculator.h"
#include "SkillEffectSystem.h"
#include "Automaton.h"
#include "AutomatonStorage.h"
#include "Party.h"
#include "Enemy.h"
#include "Player.h"
#include "Graphic.h"
#include "WorldRenderer.h"
#include "TitleRenderer.h"
#include "GameOverRenderer.h"
#include "LegacyGameTypes.h"
#include "Controller.h"
#include "EncounterManager.h"
#include "EncounterSpec.h"
#include "Exploration.h"
#include "FieldEventHandler.h"
#include "Vibration.h"
#include "Skill.h"
#include "SkillCatalog.h"
#include "Inventory.h"
#include "Map.h"
#include "WorldProgress.h"
#include "FloorManager.h"
#include "Settings.h"
#include "AudioManager.h"
#include "BgmCue.h"
#include "Storage.h"
#include "SaveManager.h"
#include "ImageManager.h"
#include "GameSession.h"
#include "Battle.h"
#include "BattleRenderer.h"
#include "MenuFlow.h"
#include "MainMenuRenderer.h"
#include "ItemRenderer.h"
#include "SkillRenderer.h"
#include "EquipmentRenderer.h"
#include "EvolutionRenderer.h"
#include "ShopRenderer.h"
#include "InnRenderer.h"
#include "StorageRenderer.h"
#include "SettingsRenderer.h"
#include "MemoryMonitor.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"


#include <vector>
#include <algorithm>
#include <utility>

struct Game {
  GameSession* session = nullptr;
  Graphic* graphic;
  WorldRenderer worldRenderer;
  TitleRenderer titleRenderer;
  GameOverRenderer gameOverRenderer;
  Controller* controller;
  AudioManager* music;
  Storage* sd;
  MenuFlow* menu;
  MainMenuRenderer mainMenuRenderer;
  ItemRenderer itemRenderer;
  SkillRenderer skillRenderer;
  EquipmentRenderer equipmentRenderer;
  EvolutionRenderer evolutionRenderer;
  ShopRenderer shopRenderer;
  InnRenderer innRenderer;
  StorageRenderer storageRenderer;
  SettingsRenderer settingsRenderer;
  EncounterManager* encounterManager;
  Exploration exploration;
  FieldEventHandler fieldEventHandler;
  FloorManager floorManager;
  GameState state;
  FieldEvent currentEvent = {FieldEventType::NONE, 0, 0};
  SaveManager* saveManager;
  Battle* battle;
  BattleRenderer battleRenderer;
  bool requestRegenerate;
  Vibration* vibration;
  ImageManager* imgMgr;
  MemoryMonitor memMonitor;
  Settings settings;
  void drawMenuFlow();
  void startBattleEncounter(const EncounterSpec& spec, const BattleBgmHooks& bgm);
  void applyFloorMapConfig();
  void loadGameAndRegenerateLegacy(LoadGameResult& result);
  static void boot();
  void update();
};

extern Game* ctx;
