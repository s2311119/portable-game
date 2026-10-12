// main.ino
#include "Game.h"
#include <SPI.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "bmp.h"
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
#include <utility>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"


#define LGFX_USE_SD 1
#define H_TRANSPARENT 0xFFFF
// -------------------------
// Map / Game / Graphic 等（省略せずそのまま）
// -------------------------
// ここは質問者が既に持っているゲームロジック部分をそのまま使います。
// （読みやすさのため必要最小限のみ再掲します）
// main.ino

#include <vector>      // std::vector を使うために必要
#include <algorithm>   // std::min, std::max を使うために必要

//-----------------------------
/*
ストライカー系: 39->44->49->{54, 55} ->{{61, 62}, {63, 64}}
ガーディアン系: 40->45->50->{56, 57} ->{{65, 66}, {67. 68}}
クリッパー系: 41->46->51->58 ->{69, 70}
メンダ―系: 42->47->52->59 ->{71, 72}
ジャマー系: 43->48->53->60->{73, 74}
*/
//-----------------------------

// グローバル（core0 から参照するけど SD 操作は core1 のみ）





// main.ino


// main.ino の Item クラスの後に追加












// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------





// -------------------------
// AUDIO / SD (core1 に限定)
// -------------------------


// ---------
// saveクラス
// ---------



// -------------------------
// BATTLE クラス
// -------------------------


Game* ctx = nullptr;

// main.ino (setup 関数)

void setup() {
  Game::boot();
}

void loop() {
  ctx->update();
}