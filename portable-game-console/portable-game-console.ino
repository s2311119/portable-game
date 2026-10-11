// main.ino
#include <SPI.h>
#include <SdFat.h>
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
#include "MUSIC.h"
#include "Sd.h"
#include "SaveManager.h"
#include "ImageManager.h"
#include "GameSession.h"
#include "Battle.h"
#include "MemoryMonitor.h"
#include <utility>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "pico/sync.h"
critical_section_t sdLock;

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



const int learnableSkillIDs[] = {
  // --- 攻撃系 ---
  1,  // ノイズ・ブラスト (TP:12)
  2,  // レゾナンス・ウェイブ (TP:25, 全体)
  3,  // ディソナンス (TP:20)
  
  // --- 回復・治療系 ---
  4,  // ファースト・エイド (TP:8)
  5,  // ピュリファイ (TP:10, 治療)
  6,  // ヒーリング・サークル (TP:30, 全体回復)

  // --- 補助・妨害系 ---
  7,  // チューニング・アタック (TP:15, ATK↑)
  8,  // チューニング・ガード (TP:15, DEF↑)
  9,  // アレグロ (TP:15, 全体SPD↑)
  10, // ラレンタンド (TP:12, 敵SPD↓)
  11, // ノイズ・ジャミング (TP:18, 全体ATK↓)

  // --- 奥義 (コスト高め) ---
  12, // グランド・シンフォニー (TP:60, 全体特大)
  13  // 完全調律 (TP:80, 全体全快)
};
const int learnableSkillCount = sizeof(learnableSkillIDs) / sizeof(int);









// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------





// -------------------------
// AUDIO / SD (core1 に限定)
// -------------------------
bool audio_timer_callback(struct repeating_timer *t);


namespace field_input_adapter {
bool pressed(void* context, FieldDirection direction) {
  Controller& ctrl = *static_cast<Controller*>(context);
  switch (direction) {
    case FieldDirection::UP: return ctrl.pressed(ctrl.BTN_UP);
    case FieldDirection::DOWN: return ctrl.pressed(ctrl.BTN_DOWN);
    case FieldDirection::LEFT: return ctrl.pressed(ctrl.BTN_LEFT);
    case FieldDirection::RIGHT: return ctrl.pressed(ctrl.BTN_RIGHT);
  }
  return false;
}
} // namespace field_input_adapter

namespace field_random_adapter {
long drawMax(void*, long max) { return random(max); }
long drawRange(void*, long min, long max) { return random(min, max); }
} // namespace field_random_adapter

// ---------
// saveクラス
// ---------

class Menu {
public:
  // ★ メニューの状態を定義
  enum MenuState {
    STATE_MAIN,     // 道具、技、進化... を選ぶ
    STATE_EVOLVE,    // ★ 「進化」画面
    STATE_SKILL,
    STATE_ITEM,
    STATE_ITEM_TARGET, // ★ 道具の対象選択
    STATE_STORAGE, // ★ 預かり所画面
    STATE_SAVE_RESULT,
    // --- 装備用 ---
    STATE_EQUIP_SLOT,   // どの部位？ (武器/防具/装飾)
    STATE_EQUIP_LIST,     // 何を装備する？
    // ショップ・宿屋
    STATE_INN,          // 宿屋 (泊まりますか？)
    STATE_SHOP_CHOICE,  // 店 (買う/売る/出る)
    STATE_SHOP_BUY,     // 買う画面
    STATE_SHOP_SELL,     // 売る画面
    STATE_CONFIG
  };
  MenuState currentMenuState;

  // メインメニュー
  static const int MAIN_ITEM_COUNT = 6;
  // ★ "機構カスタマイズ" を "進化" に変更
  const char* mainItems[MAIN_ITEM_COUNT] = {"道具", "旋律", "進化", "装備", "記録", "設定"};
  int mainSelected = 0;
  // 進化画面
  int listCursor = 0;
  bool active = false;
  bool saveSucceeded = false;
  std::vector<int> shopItemList;
  int selectedItemIndex = -1; // 選択中の道具のインデックス保存用
  int storageMode = 0; // 0:メニュー選択, 1:引き出す, 2:預ける

  int equipSlot = 0;        // 0:武器, 1:防具, 2:装飾
  Menu() {
    currentMenuState = STATE_MAIN; // 初期状態
  }

  void toggle() { 
    active = !active; 
    currentMenuState = STATE_MAIN; // メニューを開くときは必ずメインから
    mainSelected = 0;
    listCursor = 0;
  }

  void update(Controller &ctrl, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, Map& map, WorldProgress& progress, MUSIC& music, Sd& sd, SaveManager& saveMgr, Settings& settings, Vibration& vib);

  bool takeSettingsSaveRequest() {
    bool requested = settingsSaveRequested;
    settingsSaveRequested = false;
    return requested;
  }
  
  void draw(Graphic* g, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, const Settings& settings) {
    if (!active) return;

    // ★ スプライト生成をやめて g->lineSprite を使う

    // --- 分割描画ループ ---
    for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {
      g->lineSprite.fillScreen(TFT_BLACK);
      int yOffset = -drawY;
      // 各要素の描画 (Y座標から drawY を引く)
      // ※ 以下、sp. を g->lineSprite. に置換
      
      if (currentMenuState == STATE_MAIN) {
        int x = 60, y = 40 - drawY; 
        int w = 200, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        for (int i = 0; i < MAIN_ITEM_COUNT; i++) {
          uint16_t color = (i == mainSelected) ? TFT_YELLOW : TFT_WHITE;
          g->lineSprite.setTextColor(color);
          g->lineSprite.setCursor(x + 20, y + 20 + i * 28);
          g->lineSprite.print(mainItems[i]);
        }

      } else if (currentMenuState == STATE_EVOLVE) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("進化 (誰を？)");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          int memberCount = party.memberCount();
          if (memberCount == 0) {
              g->lineSprite.setCursor(x + 20, y + 55);
              g->lineSprite.print("仲間がいません");
          } else {
              for (int i = 0; i < memberCount; i++) {
                  if (i >= 5) break; 
                  const Automaton& ally = party.member(i);
                  uint16_t color = (i == listCursor) ? TFT_YELLOW : TFT_WHITE;
                  g->lineSprite.setTextColor(color);
                  g->lineSprite.setCursor(x + 20, y + 55 + i * 28);
                  g->lineSprite.printf("%s Lv%d HP%d/%d", ally.state().name(), ally.state().progress().level(), ally.state().stats().getHp(), ally.state().stats().getMaxHp());
              }
          }

      } else if (currentMenuState == STATE_SKILL) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("旋律 (何を習得？)");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          int itemsPerPage = 3;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = startIdx + itemsPerPage;
          if (endIdx > learnableSkillCount) endIdx = learnableSkillCount;

          for (int i = startIdx; i < endIdx; i++) {
              Skill skill = SkillCatalog::get(learnableSkillIDs[i]);
              int scrollId = 200 + skill.id;
              bool hasScroll = (inventory.countItem(scrollId) > 0);
              bool alreadyLearned = false;
              alreadyLearned = hero.state().skills().has(skill.id);
              uint16_t color = TFT_WHITE;
              if (i == listCursor) color = TFT_YELLOW;
              else if (alreadyLearned) color = TFT_GREEN;
              else if (!hasScroll) color = TFT_LIGHTGREY;

              g->lineSprite.setTextColor(color);
              g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
              String statusStr = "";
              if (alreadyLearned) statusStr = "(済)";
              g->lineSprite.printf("%s%s (TP:%d)", skill.name, statusStr.c_str(), skill.tpCost);
          }
          
          g->lineSprite.drawLine(x, y + 140, x + w, y + 140, TFT_WHITE);
          int targetSkillId = learnableSkillIDs[listCursor];
          int reqScrollId = 200 + targetSkillId;
          int scrollCount = inventory.countItem(reqScrollId);
          g->lineSprite.setCursor(x + 20, y + 155);
          g->lineSprite.setTextColor(TFT_WHITE);
          bool isLearned = false;
          isLearned = hero.state().skills().has(targetSkillId);
          if (isLearned) { g->lineSprite.setTextColor(TFT_GREEN); g->lineSprite.print("習得済みです"); } 
          else if (scrollCount > 0) { g->lineSprite.setTextColor(TFT_CYAN); g->lineSprite.printf("習得可能！ (所持: %d)", scrollCount); } 
          else { g->lineSprite.setTextColor(TFT_RED); g->lineSprite.setTextSize(1); g->lineSprite.setCursor(x + 20, y + 160); g->lineSprite.printf("必要: %s", ItemCatalog::get(reqScrollId).name); }

      } else if (currentMenuState == STATE_ITEM) {
        int x = 10, y = 40 - drawY; 
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("道具 (一覧)");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        int itemCount = inventory.items.size();
        if (itemCount == 0) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("道具を持っていません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = startIdx + itemsPerPage;
          if (endIdx > itemCount) endIdx = itemCount;

          for (int i = startIdx; i < endIdx; i++) {
            int id = inventory.items[i].item.id;
            bool isUsable = (id >= 100 && id < 200);
            uint16_t color = TFT_WHITE;
            if (i == listCursor) color = TFT_YELLOW;
            else if (!isUsable) color = TFT_LIGHTGREY;

            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            if (id >= 200 && id < 300) {
              // %sの巻物 と書くことで、画面上だけ結合される
              g->lineSprite.printf("%sの巻物 x%d", inventory.items[i].item.name, inventory.items[i].quantity);
            } else {
              // 通常アイテム
              g->lineSprite.printf("%s x%d", inventory.items[i].item.name, inventory.items[i].quantity);
            }
          }
        }

      } else if (currentMenuState == STATE_ITEM_TARGET) {
          int x = 10, y = 40 - drawY;
          int w = 300, h = 190;
          g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
          g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 15);
          g->lineSprite.print("誰に使いますか？");
          g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

          uint16_t color = (listCursor == 0) ? TFT_YELLOW : TFT_WHITE;
          g->lineSprite.setTextColor(color);
          g->lineSprite.setCursor(x + 20, y + 55);
          g->lineSprite.printf("調律師 HP:%d/%d", hero.state().stats().getHp(), hero.state().stats().getMaxHp());

          for (int i = 0; i < party.memberCount(); i++) {
            color = (listCursor == i + 1) ? TFT_YELLOW : TFT_WHITE;
            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x + 20, y + 55 + (i + 1) * 28);
            const auto* s = &party.member(i).state();
            g->lineSprite.printf("%s HP:%d/%d", s->name(), s->stats().getHp(), s->stats().getMaxHp());
          }

      } else if (currentMenuState == STATE_STORAGE) {
          g->lineSprite.fillScreen(TFT_BLACK); 
          g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setTextSize(2);
          g->lineSprite.setCursor(10, 10 - drawY);
          g->lineSprite.print("=== 預かり所 ===");

          if (storageMode == 0) {
              g->lineSprite.setCursor(40, 50 - drawY);
              g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE); g->lineSprite.print("仲間を引き出す");
              g->lineSprite.setCursor(40, 80 - drawY);
              g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE); g->lineSprite.print("仲間を預ける");
              g->lineSprite.setTextColor(TFT_WHITE);
              g->lineSprite.setCursor(40, 130 - drawY);
              g->lineSprite.printf("パーティー: %d/%d人", party.memberCount(), Party::MAX_MEMBERS);
              g->lineSprite.setCursor(40, 150 - drawY);
              g->lineSprite.printf("預かり所: %d人", storage.memberCount());
          }
          else if (storageMode == 1) {
              g->lineSprite.setCursor(10, 35 - drawY); g->lineSprite.print("誰を引き出しますか？");
              if (party.memberCount() >= Party::MAX_MEMBERS) { g->lineSprite.setTextColor(TFT_RED); g->lineSprite.print(" (満員)"); }
              int count = storage.memberCount();
              if (count == 0) { g->lineSprite.setCursor(30, 60 - drawY); g->lineSprite.setTextColor(TFT_LIGHTGREY); g->lineSprite.print("誰もいません"); }
              
              int itemsPerPage = 6; 
              int startIdx = 0;
              if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
              int endIdx = startIdx + itemsPerPage;
              if (endIdx > count) endIdx = count;

              for(int i = startIdx; i < endIdx; i++) {
                  g->lineSprite.setCursor(30, 60 + (i - startIdx) * 25 - drawY);
                  g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
                  g->lineSprite.printf(
                      "%s Lv%d",
                      storage.member(i).state().name(),
                      storage.member(i).state().progress().level()
                  );
              }
          }
          else if (storageMode == 2) {
              g->lineSprite.setCursor(10, 35 - drawY); g->lineSprite.print("誰を預けますか？");
              int count = party.memberCount();
              if (count == 0) { g->lineSprite.setCursor(30, 60 - drawY); g->lineSprite.print("誰もいません"); }
              for(int i=0; i<count; i++) {
                  g->lineSprite.setCursor(30, 60 + i*25 - drawY);
                  g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
                  g->lineSprite.printf(
                      "%s Lv%d",
                      party.member(i).state().name(),
                      party.member(i).state().progress().level()
                  );
              }
          }
      } else if (currentMenuState == STATE_SAVE_RESULT) {
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
      }else if (currentMenuState == STATE_EQUIP_SLOT) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        CharacterState* s = &hero.state(); // 主人公固定
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("装備 (%s)", s->name()); 
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        String wName = (s->equipmentState().weaponId() == -1) ? "なし" : ItemCatalog::get(s->equipmentState().weaponId()).name;
        String aName = (s->equipmentState().armorId() == -1) ? "なし" : ItemCatalog::get(s->equipmentState().armorId()).name;
        String acName = (s->equipmentState().accessoryId() == -1) ? "なし" : ItemCatalog::get(s->equipmentState().accessoryId()).name;

        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 60); g->lineSprite.printf("武器: %s", wName.c_str());

        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 90); g->lineSprite.printf("防具: %s", aName.c_str());

        g->lineSprite.setTextColor(listCursor == 2 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 120); g->lineSprite.printf("装飾: %s", acName.c_str());
        
        g->lineSprite.setTextColor(TFT_CYAN);
        g->lineSprite.setCursor(x + 20, y + 160);
        g->lineSprite.printf("ATK:%d DEF:%d", s->attack(), s->defense());
      }else if (currentMenuState == STATE_EQUIP_LIST) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("装備するアイテム");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        std::vector<int> equippableIndices;
        Item::ItemType targetType = Item::ITEM_WEAPON; // Item::をつける
        if (equipSlot == 1) targetType = Item::ITEM_ARMOR;
        if (equipSlot == 2) targetType = Item::ITEM_ACCESSORY;

        for(int i=0; i<inventory.items.size(); i++) {
          if (inventory.items[i].item.type == targetType) {
            equippableIndices.push_back(i);
          }
        }

        if (equippableIndices.empty()) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("装備できるものがありません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = min(startIdx + itemsPerPage, (int)equippableIndices.size());

          for (int i = startIdx; i < endIdx; i++) {
            int idx = equippableIndices[i];
            Item& it = inventory.items[idx].item;
            
            g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            
            String valStr = "";
            if (it.type == Item::ITEM_WEAPON) valStr = "ATK+" + String(it.value);
            else if (it.type == Item::ITEM_ARMOR) valStr = "DEF+" + String(it.value);
            
            g->lineSprite.printf("%s (%s)", it.name, valStr.c_str());
          }
        }
      }else if (currentMenuState == STATE_INN) {
        int x = 60, y = 80 + yOffset;
        g->lineSprite.fillRect(x, y, 200, 100, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, 200, 100, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 20);
        g->lineSprite.print("一泊 10G です");
        
        g->lineSprite.setCursor(x + 40, y + 50);
        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("泊まる");
        
        g->lineSprite.setCursor(x + 40, y + 75);
        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("やめる");
      }else if (currentMenuState == STATE_SHOP_CHOICE) {
        int x = 60, y = 80 + yOffset;
        g->lineSprite.fillRect(x, y, 200, 120, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, 200, 120, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 10);
        g->lineSprite.print("いらっしゃい");
        
        g->lineSprite.setCursor(x + 40, y + 40);
        g->lineSprite.setTextColor(listCursor == 0 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("買う");
        
        g->lineSprite.setCursor(x + 40, y + 65);
        g->lineSprite.setTextColor(listCursor == 1 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("売る");

        g->lineSprite.setCursor(x + 40, y + 90);
        g->lineSprite.setTextColor(listCursor == 2 ? TFT_YELLOW : TFT_WHITE);
        g->lineSprite.print("出る");
      }else if (currentMenuState == STATE_SHOP_BUY) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("買う (所持金:%dG)", inventory.money);
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        const int shopItems[] = {100, 101, 103, 104, 300, 301, 400, 401, 500};
        int itemCount = shopItemList.size();
        
        // スクロール
        int itemsPerPage = 5;
        int startIdx = 0;
        if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
        int endIdx = min(startIdx + itemsPerPage, itemCount);

        for (int i = startIdx; i < endIdx; i++) {
          // リストからID取得
          int itemId = shopItemList[i];
          Item it = ItemCatalog::get(itemId);

          g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
          g->lineSprite.printf("%s (%dG)", it.name, it.price);
        }
      }else if (currentMenuState == STATE_SHOP_SELL) {
        int x = 10, y = 40 + yOffset;
        int w = 300, h = 190;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.printf("売る (所持金:%dG)", inventory.money);
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        int itemCount = inventory.items.size();
        if (itemCount == 0) {
          g->lineSprite.setCursor(x + 20, y + 55); g->lineSprite.print("売れるものがありません");
        } else {
          int itemsPerPage = 5;
          int startIdx = 0;
          if (listCursor >= itemsPerPage) startIdx = listCursor - (itemsPerPage - 1);
          int endIdx = min(startIdx + itemsPerPage, itemCount);

          for (int i = startIdx; i < endIdx; i++) {
            Item& it = inventory.items[i].item;
            int sellPrice = it.price / 2;
            
            g->lineSprite.setTextColor(listCursor == i ? TFT_YELLOW : TFT_WHITE);
            g->lineSprite.setCursor(x + 20, y + 55 + (i - startIdx) * 28);
            g->lineSprite.printf("%s x%d (%dG)", it.name, inventory.items[i].quantity, sellPrice);
          }
        }
      }else if (currentMenuState == STATE_CONFIG) {
        int x = 60, y = 40 + yOffset;
        int w = 200, h = 160;
        g->lineSprite.fillRect(x, y, w, h, TFT_DARKGREY);
        g->lineSprite.drawRect(x, y, w, h, TFT_WHITE);
        
        g->lineSprite.setTextSize(2);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 20, y + 15);
        g->lineSprite.print("環境設定");
        g->lineSprite.drawLine(x, y + 40, x + w, y + 40, TFT_WHITE);

        // 項目描画ヘルパー
        auto drawItem = [&](int idx, String label, String value) {
          if (listCursor == idx) g->lineSprite.setTextColor(TFT_YELLOW);
          else g->lineSprite.setTextColor(TFT_WHITE);
          g->lineSprite.setCursor(x + 20, y + 55 + idx * 25);
          g->lineSprite.print(label);
          g->lineSprite.setCursor(x + 140, y + 55 + idx * 25);
          g->lineSprite.print(value);
        };

        // 1. BGM (引数の settings を使用)
        String bgmValStr = String(settings.getBgmVolume());
        drawItem(0, "BGM", bgmValStr);

        // 2. SE (引数の settings を使用)
        String seValStr = String(settings.getSeVolume());
        drawItem(1, "SE", seValStr);

        // 3. 振動 (引数の settings を使用)
        String vibValStr = settings.isVibrationEnabled() ? "ON" : "OFF";
        drawItem(2, "振動", vibValStr);

        // 4. 戻る
        if (listCursor == 3) g->lineSprite.setTextColor(TFT_YELLOW);
        else g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(x + 60, y + 130);
        g->lineSprite.print("戻る");
      }
        
      // ★★★ 画面に転送 ★★★
      g->lineSprite.pushSprite(0, drawY);
    }
    
  }

private:
  bool settingsSaveRequested = false;
};

// -------------------------
// BATTLE クラス
// -------------------------


struct GameContext {
  GameSession* session = nullptr;
  Graphic* graphic;
  Controller* controller;
  MUSIC* music;
  Sd* sd;
  Menu* menu;
  EncounterManager* encounterManager;
  Exploration exploration;
  FieldEventHandler fieldEventHandler;
  FloorManager floorManager;
  GameState state;
  FieldEvent currentEvent = {FieldEventType::NONE, 0, 0};
  SaveManager* saveManager;
  Battle* battle;
  bool requestRegenerate;
  Vibration* vibration;
  ImageManager* imgMgr;
  MemoryMonitor memMonitor;
  Settings settings;
};

GameContext* ctx = nullptr;

void applyFloorMapConfig(GameContext* ctx);

void applyFloorMapConfig(GameContext* ctx)
{
  FloorMapConfig config =
      ctx->floorManager.getConfig(ctx->session->progress());

  ctx->music->switchTrack(
      config.type == Map::TYPE_TOWN ? 0 : 1,
      *ctx->sd
  );

  ctx->graphic->setFloor(
      ctx->session->progress().floor()
  );

  ctx->session->map().regenerate(
      config.width,
      config.height,
      config.type,
      ctx->session->progress().floor()
  );

  ctx->session->player().reset(ctx->session->map());
}

// タイマー割り込み関数

bool audio_timer_callback(struct repeating_timer *t) {
  MUSIC* music = ctx->music;
  
  // BGM停止中でもSEは鳴らす
  int32_t mixedSample = 0;

  // --- 1. BGMの取得 ---
  if (!music->paused) {

    // 現在の再生バッファを使い切っていたら、
    // 先に裏バッファへ切り替える
    if (music->sampleIndex + 1 >= music->playBytes) {

      if (music->fillBytes > 0) {

        // ここはタイマーISR内なので、
        // noInterrupts()/interrupts() は不要
        volatile uint8_t* tmp = music->playBuf;
        music->playBuf = music->fillBuf;
        music->fillBuf = (uint8_t*)tmp;

        music->playBytes = music->fillBytes;
        music->fillBytes = 0;
        music->sampleIndex = 0;

        // 空いた裏バッファの補充をcore1ループへ要求
        music->needs_refill = true;
      }
      else {
        // 裏バッファの準備が間に合っていない
        music->needs_refill = true;
      }
    }

    // バッファを切り替えた場合も、
    // 同じタイマー割り込み内でそのまま次の1サンプルを読む。
    // これによりバッファ境界に無音サンプルを挟まない。
    if (music->sampleIndex + 1 < music->playBytes) {

      uint8_t lo =
          music->playBuf[music->sampleIndex];

      uint8_t hi =
          music->playBuf[music->sampleIndex + 1];

      int16_t bgm =
          (int16_t)(lo | (hi << 8));

      bgm =
          (int16_t)(bgm * music->bgmVolume);

      mixedSample += bgm;

      music->sampleIndex += 2;
    }
  }

  // --- 2. SE (シンセサイザー生成) ★ここが変わった！ ---
  if (music->seType != MUSIC::OFF) {
    int16_t seWave = 0;

    // 音量の減衰
    music->seVol -= music->seVolDecay;
    if (music->seVol <= 0) {
      music->seType = MUSIC::OFF; // 音量0で終了
      music->seVol = 0;
    } else {
      // 周波数の変化 (スライド)
      music->seFreq += music->seFreqSlide;
      
      // 波形生成
      if (music->seType == MUSIC::SQUARE) {
        // 矩形波: 位相を進めて High/Low を切り替え
        // 16000Hz動作なので、位相は Freq / 16000 ずつ進む
        music->sePhase += music->seFreq / 16000.0f;
        if (music->sePhase >= 1.0f) music->sePhase -= 1.0f;
        
        seWave = (music->sePhase < 0.5f) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      } 
      else if (music->seType == MUSIC::NOISE) {
        // ノイズ: 乱数
        // (高速化のため簡易的な乱数生成)
        static uint16_t lfsr = 0xACE1;
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        seWave = (lfsr & 1) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      }
    }
    
    mixedSample += seWave;
  }

  // --- 3. クリッピング (音割れ防止) ---
  if (mixedSample > 32767) mixedSample = 32767;
  if (mixedSample < -32768) mixedSample = -32768;

  // --- 4. DACへ出力 ---
  // 16bit signed -> 12bit unsigned (0〜4095)
  uint16_t usample = (uint16_t)(mixedSample + 32768) >> 6;
  if (usample > 4095) usample = 4095;
  music->setDAC_ISR(usample);
  
  return true;
}

// -------------------------
// core1 のエントリポイント（オーディオ専用）
// -------------------------
void core1_main() {

  // -------------------------------------------------
  // 音声専用タイマーをcore1上に作成
  //
  // デフォルトalarm poolは使わず、
  // このcore1上で独自のalarm poolを作ることで、
  // audio_timer_callbackもcore1側のIRQとして実行する。
  // -------------------------------------------------
  static alarm_pool_t* audioAlarmPool =
      alarm_pool_create_with_unused_hardware_alarm(1);

  static repeating_timer_t audioTimer;

  // 16kHzの理想周期は62.5us。
  // repeating_timerは整数us指定なので、
  // 近い63usを使用する。
  //
  // 負値にすることで
  // 「callback終了から63us後」ではなく
  // 「callback開始から63usごと」に呼ばれる。
  bool timerStarted =
      alarm_pool_add_repeating_timer_us(
          audioAlarmPool,
          -63,
          audio_timer_callback,
          nullptr,
          &audioTimer
      );

  // タイマー生成に失敗した場合は、
  // 不定状態でゲームを続けない。
  if (!timerStarted) {
    while (true) {
      tight_loop_contents();
    }
  }

  while (true) {
    ctx->sd->performPendingSwitch(*ctx->music);
    ctx->sd->refile(*ctx->music);

    tight_loop_contents();
  }
}

void Menu::update(Controller &ctrl, Inventory& inventory, Player& hero, Party& party, AutomatonStorage& storage, Map& map, WorldProgress& progress, MUSIC& music, Sd& sd, SaveManager& saveMgr, Settings& settings, Vibration& vib) {
  if (!active) return; // メニューが非表示なら何もしない
  bool aPressed = ctrl.pressedDebounced(ctrl.BTN_A);
  bool bPressed = ctrl.pressedDebounced(ctrl.BTN_B);
  bool upPressed = ctrl.pressedDebounced(ctrl.BTN_UP);
  bool downPressed = ctrl.pressedDebounced(ctrl.BTN_DOWN);
  bool rightPressed = ctrl.pressedDebounced(ctrl.BTN_RIGHT);
  bool leftPressed = ctrl.pressedDebounced(ctrl.BTN_LEFT);
  
  // --- Bボタン（キャンセル）処理 ---
  if (bPressed) {
    music.playSE(1);
    if (currentMenuState == STATE_ITEM_TARGET) {
      currentMenuState = STATE_ITEM; // 対象選択から道具一覧へ戻る
      return;
    }else if (currentMenuState == STATE_STORAGE) {
      if (storageMode != 0) storageMode = 0; // モード選択に戻る
      else active = false; // メニューを閉じる (イベント終了)
      return;
    }else if (currentMenuState == STATE_EQUIP_LIST) {
      currentMenuState = STATE_EQUIP_SLOT; 
    }else if (currentMenuState == STATE_SHOP_BUY || currentMenuState == STATE_SHOP_SELL) {
      currentMenuState = STATE_SHOP_CHOICE; // 選択画面に戻る
    }else if (currentMenuState == STATE_SHOP_CHOICE) {
      active = false; // 店を出る
    }else if (currentMenuState == STATE_INN) {
      active = false; // 宿屋を出る
    }else if (currentMenuState == STATE_EQUIP_SLOT||currentMenuState == STATE_EVOLVE || currentMenuState == STATE_SKILL || currentMenuState == STATE_ITEM || currentMenuState == STATE_CONFIG) {
      // 進化・旋律画面でBを押したらメインに戻る
      if (currentMenuState == STATE_CONFIG) settingsSaveRequested = true;
      currentMenuState = STATE_MAIN;
    } else if (currentMenuState == STATE_MAIN) {
      // メインメニューでBを押したらメニューを閉じる
      active = false;
    }
    return; // Bボタンが押されたフレームは他の操作を無視
  }

  // --- 状態別 Aボタン（決定）とカーソル移動 ---
  switch (currentMenuState) {
    
    case STATE_MAIN: {
      // --- メインメニューの操作 ---
      if (upPressed){
        mainSelected = (mainSelected - 1 + MAIN_ITEM_COUNT) % MAIN_ITEM_COUNT;
        music.playSE(2);
      }
      if (downPressed){
        mainSelected = (mainSelected + 1) % MAIN_ITEM_COUNT;
        music.playSE(2);
      }
      if (aPressed) {
        music.playSE(0);
        if (mainSelected == 0) {
          currentMenuState = STATE_ITEM;
          listCursor = 0;
        }else if (mainSelected == 1) { // 「旋律」が選ばれた
          currentMenuState = STATE_SKILL;
          listCursor = 0; 
        }
        else if (mainSelected == 2) { // 「進化」が選ばれた
          currentMenuState = STATE_EVOLVE;
          listCursor = 0;
        }else if (mainSelected == 3) { 
          currentMenuState = STATE_EQUIP_SLOT; 
          listCursor = 0; 
        }else if (mainSelected == 4) { // 「記録」
          saveSucceeded =
              saveMgr.saveGame(sd, music, map, progress, hero, party, storage, inventory);

          currentMenuState = STATE_SAVE_RESULT;
        }else if (mainSelected == 5) {
          currentMenuState = STATE_CONFIG;
          listCursor = 0;
        }
      }
      break;
    }
    case STATE_ITEM: {
      // 全アイテムを対象にする
      int itemCount = inventory.items.size();

      if (itemCount > 0) {
        if (upPressed){
          listCursor = (listCursor - 1 + itemCount) % itemCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % itemCount;
          music.playSE(2);
        }
        if (aPressed) {
          // カーソル位置のアイテムIDを確認
          int id = inventory.items[listCursor].item.id;
          
          // ★ 消費アイテム (100〜199) の場合のみ使用可能
          if (id >= 100 && id < 200) {
            music.playSE(0);
            selectedItemIndex = listCursor; // そのままインデックスとして保存
            currentMenuState = STATE_ITEM_TARGET;
            listCursor = 0; // ターゲット選択のカーソル初期化
          } else {
            music.playSE(1);
            // 使えないアイテムは何もしない
          }
        }
      }
      break;
    }
    case STATE_ITEM_TARGET: {
      // 対象リスト: [0]主人公, [1~]仲間
      int totalTargets = 1 + party.memberCount();

      if (upPressed){
        listCursor = (listCursor - 1 + totalTargets) % totalTargets;
        music.playSE(2);
      }
      if (downPressed){
        listCursor = (listCursor + 1) % totalTargets;
        music.playSE(2);
      }

      if (aPressed) {
        music.playSE(0);
        // 対象のStatus特定
        CharacterState* targetStatus = nullptr;
        if (listCursor == 0) targetStatus = &hero.state();
        else targetStatus = &party.memberState(listCursor - 1);

        // アイテム効果適用
        int itemId = inventory.items[selectedItemIndex].item.id;
        
        if (itemId == 100) targetStatus->heal(50);
        else if (itemId == 101) targetStatus->heal(200);
        else if (itemId == 102) targetStatus->heal(9999);
        else if (itemId == 103) targetStatus->restoreTP(20);
        else if (itemId == 104) targetStatus->CureStatus(true);

        // 消費して戻る
        inventory.removeByIndex(selectedItemIndex);
        currentMenuState = STATE_ITEM;
        listCursor = 0;
      }
      break;
    }
    case STATE_STORAGE: {
      // storageMode: 0=選択, 1=引き出す(Withdraw), 2=預ける(Deposit)
      
      if (storageMode == 0) {
        // メニュー選択 (上:引き出す, 下:預ける)
        if (upPressed || downPressed) {
          listCursor = (listCursor == 0) ? 1 : 0;
          music.playSE(2);
        }
        if (aPressed) {
          storageMode = (listCursor == 0) ? 1 : 2; // 0なら引き出す、1なら預ける
          listCursor = 0;
          music.playSE(0);
        }
      }
      else if (storageMode == 1) { // 引き出す (Storage -> Party)
        int sCount = storage.memberCount();
        if (sCount > 0) {
          if (upPressed){
            listCursor = (listCursor - 1 + sCount) % sCount;
            music.playSE(2);
          }
          if (downPressed){
            listCursor = (listCursor + 1) % sCount;
            music.playSE(2);
          }
          
          if (aPressed) {
            if (party.memberCount() < Party::MAX_MEMBERS) {
              storage.withdrawMember(listCursor, party);
              listCursor = 0; // リセット
              music.playSE(0);
            }else{
              music.playSE(1);
            }
          }
        }
      }
      else if (storageMode == 2) { // 預ける (Party -> Storage)
        int pCount = party.memberCount();
        if (pCount > 0) {
          if (upPressed){
            listCursor = (listCursor - 1 + pCount) % pCount;
            music.playSE(2);
          }
          if (downPressed){
            listCursor = (listCursor + 1) % pCount;
            music.playSE(2);
          }
          if (aPressed) {
            party.depositMember(listCursor, storage);
            if (listCursor >= party.memberCount()) listCursor = 0;
            music.playSE(0);
          }
        }
      }
      break;
    }
    case STATE_EVOLVE: {
      // --- 進化画面（仲間選択）の操作 ---
      int memberCount = party.memberCount();
      if (memberCount > 0) { // 仲間が1人以上いる
        if (upPressed){
          listCursor = (listCursor - 1 + memberCount) % memberCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % memberCount;
          music.playSE(2);
        }

        if (aPressed) {
          const ProgressionResult evolution =
              CharacterProgressionSystem::tryItemEvolution(
                  party.memberState(listCursor), inventory,
                  [](void* context) {
                    static_cast<MUSIC*>(context)->playSE(1);
                  },
                  &music);
          const bool itemUsed = evolution.evolved;
          if (itemUsed) music.playSE(0);

          // --- 進化後の後処理 ---
          if (itemUsed) {
            // ★ ここにあった erase は削除しました (上で既に消しているため)
            
            // カーソル位置を安全な場所に補正 (アイテムが減ってズレる可能性があるため)
            int newItemCount = inventory.items.size();
            // アイテムリストの方のカーソルではなく、仲間リストのカーソル(listCursor)なので
            // 実はここはあまり関係ないですが、念のためバグ防止
            if (listCursor >= memberCount) {
                listCursor = memberCount - 1;
            }
            // (もしアイテム画面ならアイテムリストのカーソル補正が必要ですが、
            //  ここは進化画面なので、仲間リストのカーソルはそのままで大丈夫です)
          }
        } // Aボタン処理 終了
      } // itemCount > 0 終了
      break;
    }
    case STATE_SKILL: {
      // --- ★ 旋律画面の操作 ★ ---
      if (learnableSkillCount > 0) {
        // カーソル移動
        if (upPressed) {
          listCursor = (listCursor - 1 + learnableSkillCount) % learnableSkillCount;
          music.playSE(2);
        }
        if (downPressed){
          listCursor = (listCursor + 1) % learnableSkillCount;
          music.playSE(2);
        }

        // Aボタン（決定）
        if (aPressed) {
          int skillIdToLearn = learnableSkillIDs[listCursor];
          int requiredScrollId = 200 + skillIdToLearn; // 巻物のID

          // ★ 1. すでに覚えているかチェック
          bool alreadyLearned = false;
          alreadyLearned = hero.state().skills().has(skillIdToLearn);

          if (alreadyLearned) {
            music.playSE(1);
            // すでに覚えているなら何もしない (SEを鳴らすならここ)
          } 
          // ★ 2. 持っているかチェック
          else if (inventory.countItem(requiredScrollId) > 0) {
              // 消費して習得
              inventory.removeItem(requiredScrollId);
              hero.state().learnSkill(skillIdToLearn);
              music.playSE(0);
              // (※ここで「覚えた！」というフラグを立てて、draw側でメッセージを出すと親切です)
          } else {
              // (巻物がない)
              music.playSE(1);
          }
        }
      }
      break;
    }
    case STATE_SAVE_RESULT:
      if (aPressed) { active = false; music.playSE(1);} // 閉じる
      break;
    case STATE_EQUIP_SLOT: {
      // 0:武器, 1:防具, 2:装飾
      if (upPressed)   { listCursor = (listCursor - 1 + 3) % 3; music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % 3; music.playSE(2); }
      if (aPressed) {
          music.playSE(0);
          equipSlot = listCursor; 
          currentMenuState = STATE_EQUIP_LIST;
          listCursor = 0;
      }
      break;
    }
    case STATE_EQUIP_LIST: {
      std::vector<int> equippableIndices;
      Item::ItemType targetType = Item::ITEM_WEAPON;
      if (equipSlot == 1) targetType = Item::ITEM_ARMOR;
      if (equipSlot == 2) targetType = Item::ITEM_ACCESSORY;

      for(int i=0; i<inventory.items.size(); i++) {
        if (inventory.items[i].item.type == targetType) {
          equippableIndices.push_back(i);
        }
      }
      
      int count = equippableIndices.size();
      if (count > 0) {
        if (upPressed)   { listCursor = (listCursor - 1 + count) % count; music.playSE(2); }
        if (downPressed) { listCursor = (listCursor + 1) % count; music.playSE(2); }
        
        if (aPressed) {
          music.playSE(0);
          int invIndex = equippableIndices[listCursor];
          Item newItem = inventory.items[invIndex].item; 

          // ★ 主人公固定
          CharacterState* s = &hero.state();
          
          // 1. 外す (インベントリに戻す)
          int oldId = -1;
          if (equipSlot == 0) oldId = s->equipmentState().weaponId();
          if (equipSlot == 1) oldId = s->equipmentState().armorId();
          if (equipSlot == 2) oldId = s->equipmentState().accessoryId();

          if (oldId != -1) {
            inventory.addItem(ItemCatalog::get(oldId));
          }

          // 2. 装備する
          if (equipSlot == 0) s->equip(EquipmentSlot::Weapon, newItem.id);
          if (equipSlot == 1) s->equip(EquipmentSlot::Armor, newItem.id);
          if (equipSlot == 2) s->equip(EquipmentSlot::Accessory, newItem.id);

          // 3. 消費
          inventory.removeByIndex(invIndex);

          currentMenuState = STATE_EQUIP_SLOT;
        }
      }
      break;
    }
    case STATE_INN: {
      // 0: はい, 1: いいえ
      if (upPressed || downPressed) { listCursor = (listCursor == 0) ? 1 : 0; music.playSE(2);}
      
      if (aPressed) {
        if (listCursor == 0) { // はい
          int cost = 10; // 宿泊費
          if (inventory.money >= cost) {
            music.playSE(0); // 決定音 (回復音でもOK)
            inventory.money -= cost;
            
            // 全回復
            hero.state().revive(100);
            hero.state().heal(9999); hero.state().restoreTP(9999);
            for(size_t i = 0; i < party.memberCount(); ++i) {
              CharacterState& member = party.memberState(i);
              member.revive(100);
              member.heal(9999); member.restoreTP(9999);
            }
            // (ここで「回復した！」メッセージを出したいが、簡易的に閉じる)
            active = false; 
          } else {
            music.playSE(1); // お金が足りない
          }
        } else { // いいえ
          active = false; music.playSE(1);
        }
      }
      if (bPressed) { active = false; music.playSE(1); }
      break;
    }
    case STATE_SHOP_CHOICE: {
      if (upPressed)   { listCursor = (listCursor - 1 + 3) % 3;music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % 3;music.playSE(2); }
      
      if (aPressed) {
        music.playSE(0);
        if (listCursor == 0) { // 「買う」
          currentMenuState = STATE_SHOP_BUY;
          listCursor = 0; 

          // ★★★ 商品リストの生成ロジック ★★★
          shopItemList.clear();
          
          // 1. 常備薬 (階層が進むと良い薬が入荷)
          shopItemList.push_back(100); // リペアキット
          shopItemList.push_back(103); // エネルギー缶
          shopItemList.push_back(104); // 万能オイル
          if (progress.floor() >= 10) shopItemList.push_back(101); // リペアキット中

          // 2. 階層に応じた装備品 (Tier計算)
          // 1-10F: Tier 0, 11-20F: Tier 1, ...
          int tier = (progress.floor() - 1) / 10;
          if (tier > 4) tier = 4; // 最大Tier 4 (5段階目)

          // 武器: 300 + tier
          shopItemList.push_back(300 + tier); 
          
          // 防具: 400 + tier
          shopItemList.push_back(400 + tier);

          // アクセ: たまに並ぶとか、固定とか
          if (tier >= 1) shopItemList.push_back(500); // 11F以降

        } else if (listCursor == 1) { // 売る
          currentMenuState = STATE_SHOP_SELL;
          listCursor = 0;
        } else { // 出る
          active = false;
        }
      }
      if (bPressed) { active = false; music.playSE(1); }
      break;
    }
    case STATE_SHOP_BUY: {
      int itemCount = shopItemList.size();
      
      if (upPressed)   { listCursor = (listCursor - 1 + itemCount) % itemCount; music.playSE(2); }
      if (downPressed) { listCursor = (listCursor + 1) % itemCount; music.playSE(2); }
      
      if (aPressed) {
        int itemId = shopItemList[listCursor]; // リストからID取得
        Item targetItem = ItemCatalog::get(itemId);
        
        if (inventory.money >= targetItem.price) {
          music.playSE(0); 
          inventory.money -= targetItem.price;
          inventory.addItem(targetItem);
        } else {
          music.playSE(1); 
        }
      }
      if (bPressed) {
        currentMenuState = STATE_SHOP_CHOICE;
        listCursor = 0; // カーソル位置戻す
        music.playSE(1);
      }
      break;
    }
    case STATE_SHOP_SELL: {
      int itemCount = inventory.items.size();
      if (itemCount > 0) {
        if (upPressed)   { listCursor = (listCursor - 1 + itemCount) % itemCount; music.playSE(2); }
        if (downPressed) { listCursor = (listCursor + 1) % itemCount; music.playSE(2); }
        
        if (aPressed) {
          Item& item = inventory.items[listCursor].item;
          int sellPrice = item.price / 2; // 売値は半額
          if (sellPrice < 1) sellPrice = 1;

          music.playSE(0); // チャリーン
          inventory.money += sellPrice;
          inventory.removeByIndex(listCursor);
          
          // カーソル位置調整
          if (listCursor >= inventory.items.size()) listCursor = max(0, (int)inventory.items.size() - 1);
        }
      }
      if (bPressed) {
        currentMenuState = STATE_SHOP_CHOICE;
        listCursor = 1; // 「売る」にカーソルを戻す
        music.playSE(1);
      }
      break;
    }
    case STATE_CONFIG: {
      // 項目: 0:BGM, 1:SE, 2:振動, 3:戻る
      if (upPressed)   { listCursor = (listCursor - 1 + 4) % 4; music.playSE(2);}
      if (downPressed) { listCursor = (listCursor + 1) % 4; music.playSE(2);}

      // 左右キーで値を変更
      if (leftPressed || rightPressed) {
        int change = rightPressed ? 1 : -1;
        bool valChanged = false;

        // BGM音量
        if (listCursor == 0) {
          settings.setBgmVolume(settings.getBgmVolume() + change);
          
          // 即座に反映 (ctx->music ではなく 引数の music を使う)
          music.bgmVolume = (float)settings.getBgmVolume() / 10.0f;
          valChanged = true;
        }
        // SE音量
        else if (listCursor == 1) {
          settings.setSeVolume(settings.getSeVolume() + change);
          music.setSeVolume(settings.getSeVolume());
          // SE音量を反映 (MUSICクラスに変数を持たせるか、playSEで計算)
          // music.setSeVolume(settings.getSeVolume()); // 推奨
          
          valChanged = true;
          if(valChanged) music.playSE(0); // 音量確認
        }
        // 振動 ON/OFF
        else if (listCursor == 2) {
          settings.setVibrationEnabled(!settings.isVibrationEnabled());
          valChanged = true;
          vib.setEnabled(settings.isVibrationEnabled());
          // 振動確認 (ctx->vibration ではなく 引数の vib を使う)
          if(settings.isVibrationEnabled()) vib.trigger(100);
        }
      }
      
      // Aボタン or Bボタンで戻る
      if (aPressed && listCursor == 3) {
        music.playSE(1);
        settingsSaveRequested = true;
        currentMenuState = STATE_MAIN;
      }
      break;
    }
  } // switch 終了
}

// main.ino (setup 関数)

void setup() {
  Serial.begin(115200);

  critical_section_init(&sdLock);
  
  // シード設定
  uint32_t seed = micros() ^ time_us_32();
  for (int i = 0; i < 8; ++i) {
    seed ^= (analogRead(A2) + (micros() & 0xFFFF)) << (i & 31);
    delay(1);
  }
  randomSeed(seed);
  // 1. 基本オブジェクトの生成
  ctx = new GameContext();
  ctx->graphic = new Graphic();
  ctx->controller = new Controller();
  ctx->music = new MUSIC();
  ctx->sd = new Sd();
  ctx->imgMgr = new ImageManager();
  // Graphic, SD の初期化
  ctx->graphic->init();
  ctx->sd->init(*ctx->music);
  // SaveManager 生成
  ctx->saveManager = new SaveManager();
  ctx->vibration = new Vibration();
  ctx->vibration->init();

  ctx->saveManager->loadSettings(*ctx->sd, ctx->settings);
  ctx->music->bgmVolume = (float)ctx->settings.getBgmVolume() / 10.0f;
  ctx->music->setSeVolume(ctx->settings.getSeVolume());
  ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());

  multicore_launch_core1(core1_main);
  delay(200);

  ctx->music->switchTrack(9, *ctx->sd);
  // 2. ★★★ タイトル画面処理 (ラインスプライト版) ★★★
  bool loadSuccess = false;
  bool startNewGame = false;
  int titleCursor = 0;
  
  // セーブデータがあるか確認
  bool hasSave = ctx->saveManager->hasSaveFile(*ctx->sd);
  if (!hasSave) titleCursor = 0; // なければ「はじめから」固定
  else titleCursor = 1;          // あれば「つづきから」をデフォルトに

  // フォント設定 (lineSprite用)
  ctx->graphic->lineSprite.setFont(&fonts::lgfxJapanGothic_12);

  while (!startNewGame && !loadSuccess) {
    // 入力取得
    bool up = ctx->controller->pressedDebounced(ctx->controller->BTN_UP);
    bool down = ctx->controller->pressedDebounced(ctx->controller->BTN_DOWN);
    bool a = ctx->controller->pressedDebounced(ctx->controller->BTN_A);

    if (up || down) {
      if (hasSave) titleCursor = (titleCursor == 0) ? 1 : 0;
      ctx->music->playSE(2);
    }

    if (a) {
      ctx->music->playSE(0);
      if (titleCursor == 0) {
        startNewGame = true;
        ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
      } else {
        // ロード用の仮オブジェクト生成
        ctx->session = new GameSession();

        ctx->music->paused = true;
        critical_section_enter_blocking(&sdLock);
        LoadGameResult loadResult = ctx->saveManager->loadGame(
            *ctx->sd, *ctx->music, *ctx->session);

        if (loadResult.success && loadResult.requiresMapRegeneration) {
          Map::MapType finalType = loadResult.legacyMapType;
          WorldProgress& progress = ctx->session->progress();

          // ボス階では撃破状態を優先
          if (progress.floor() >= 10 &&
              progress.floor() <= 50 &&
              progress.floor() % 10 == 0)
          {
            int bossIndex = (progress.floor() / 10) - 1;
            if (bossIndex >= 0 && bossIndex < 5) {
              if (progress.bosses().isDefeated(bossIndex)) {
                finalType = Map::TYPE_TOWN;
              } else {
                finalType = Map::TYPE_BOSS_ROOM;
              }
            }
          }

          ctx->session->map().regenerate(
              20, 20, finalType, progress.floor());
          ctx->session->player().reset(ctx->session->map());
        }

        critical_section_exit(&sdLock);
        ctx->music->paused = false;

        if (loadResult.success) {
          loadSuccess = true;
          ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
        } else {
          ctx->music->playSE(1);
          startNewGame = true; // ロード失敗時は新規
          ctx->vibration->setEnabled(ctx->settings.isVibrationEnabled());
        }
      }
    }

    // ★★★ ラインスプライトによる分割描画 ★★★
    for (int drawY = 0; drawY < Graphic::SCREEN_HEIGHT; drawY += Graphic::BLOCK_HEIGHT) {
      ctx->graphic->lineSprite.fillScreen(TFT_BLACK);
      
      // タイトルロゴ
      ctx->graphic->lineSprite.setTextSize(3);
      ctx->graphic->lineSprite.setTextColor(TFT_WHITE);
      ctx->graphic->lineSprite.setCursor(60, 80 - drawY); // ★ Y座標を drawY 分ずらす
      ctx->graphic->lineSprite.print("RPG GAME");

      // メニュー項目
      ctx->graphic->lineSprite.setTextSize(2);
      
      // 「はじめから」
      ctx->graphic->lineSprite.setTextColor(titleCursor == 0 ? TFT_YELLOW : TFT_WHITE);
      ctx->graphic->lineSprite.setCursor(100, 150 - drawY); // ★
      ctx->graphic->lineSprite.print("はじめから");

      // 「つづきから」
      if (hasSave) ctx->graphic->lineSprite.setTextColor(titleCursor == 1 ? TFT_YELLOW : TFT_WHITE);
      else ctx->graphic->lineSprite.setTextColor(TFT_DARKGREY);
      ctx->graphic->lineSprite.setCursor(100, 180 - drawY); // ★
      ctx->graphic->lineSprite.print("つづきから");

      // 画面に転送
      ctx->graphic->lineSprite.pushSprite(0, drawY);
    }
  }

  // 3. ゲーム開始準備
  if (startNewGame) {
    // 新規作成
    if (ctx->session) delete ctx->session;
    ctx->session = new GameSession();

    Map::ObjectPos startPos =
        ctx->session->map().getPlayerStartPos();

    ctx->session->player().X =
        startPos.x * Map::TILE_SIZE;

    ctx->session->player().Y =
        startPos.y * Map::TILE_SIZE;

    // 初期アイテム
    ctx->session->inventory().addItem(ItemCatalog::get(100)); // リペアキット
    
    Automaton* firstAlly = new Automaton(1, 39);
    ctx->session->party().addMember(std::move(*firstAlly));
    delete firstAlly;
  }

  // 共通初期化
  ctx->menu = new Menu();
  ctx->encounterManager = new EncounterManager();
  ctx->battle = new Battle();

  // Graphicクラスへ階層通知
  ctx->graphic->setFloor(ctx->session->progress().floor());

  // 初回描画 (ここもGraphicクラスに任せるので安全)
  int camX = ctx->session->player().X - Graphic::SCREEN_WIDTH / 2;
  int camY = ctx->session->player().Y - Graphic::SCREEN_HEIGHT / 2;
  ctx->graphic->drawFullBackground(ctx->session->map(), ctx->session->player(), camX, camY);
  if (ctx->session->map().isTown()) {
    // 集落なら bgm_0.raw
    ctx->music->switchTrack(0, *ctx->sd); 
  } else {
    // ダンジョンなら bgm_1.raw
    ctx->music->switchTrack(1, *ctx->sd);
  }

  // ゲーム状態へ移行
  ctx->state = STATE_GAME;
  // 4. 音楽スレッド起動
}

void loop() {
  ctx->memMonitor.update();
  if (ctx->requestRegenerate) {
    ctx->requestRegenerate = false;

    // MapとHeroの内容だけ再初期化（new/deleteなし）
    noInterrupts();
    ctx->session->map().regenerate(20, 20, Map::TYPE_DUNGEON, ctx->session->progress().floor());
    ctx->session->player().reset(ctx->session->map());

    ctx->graphic->setFloor(ctx->session->progress().floor());

    int cameraX = ctx->session->player().X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = ctx->session->player().Y - Graphic::SCREEN_HEIGHT / 2;
    ctx->graphic->invalidateAndRedraw(ctx->session->map(), ctx->session->player(), cameraX, cameraY);
    interrupts();
  }
  static GameState prevState = STATE_GAME;
  static int gameOverCursor = 0;
  auto& g = *ctx->graphic;
  auto& map = ctx->session->map();
  auto& hero = ctx->session->player();
  auto& ctrl = *ctx->controller;
  auto& menu = *ctx->menu;
  auto& encounterManager = *ctx->encounterManager;
  auto& inventory = ctx->session->inventory();
  auto& party = ctx->session->party();
  auto& storage = ctx->session->storage();
  auto& state = ctx->state;
  auto& music = *ctx->music;
  auto& sd = *ctx->sd;
  auto& currentEvent = ctx->currentEvent;
  auto& save = *ctx->saveManager;
  auto& vib = *ctx->vibration;
  auto& imgmgr = *ctx->imgMgr;
  auto& settings = ctx->settings;
  vib.update();
  if (state == STATE_GAME) {
    hero.oldX = hero.X;
    hero.oldY = hero.Y;

    int prevX = hero.X, prevY = hero.Y;
    uint32_t nowMs = millis();
    ExplorationInput fieldInput = {&ctrl, field_input_adapter::pressed};
    ctx->exploration.updateMovement(hero, map, nowMs, fieldInput);

    bool actionPressed = ctrl.pressedDebounced(ctrl.BTN_A);
    ExplorationResult fieldResult =
        ctx->exploration.completeStep(hero, map, prevX, prevY, actionPressed);
    bool eventOccurred = fieldResult.hasEvent;
    if (eventOccurred) {
      state = STATE_EVENT;
      currentEvent = fieldResult.event;
    }
    // 2. 移動が発生したかチェック
    bool moved = fieldResult.moved;
    // 3. 移動していて、メニューが開いていない場合のみイベントとエンカウントをチェック
    if (moved && !menu.active) {
      // 5. イベントが発生しなかった場合のみ、エンカウントをチェック
      EncounterRandom encounterRandom = {nullptr,
                                         field_random_adapter::drawMax,
                                         field_random_adapter::drawRange};
      if (!eventOccurred && encounterManager.onStep(map, encounterRandom)) {
        vib.trigger(300);
        state = STATE_BATTLE;
        if (!ctx->battle->inBattle) {
          EncounterSpec spec = encounterManager.createEncounter(
              ctx->session->progress().floor(), map, encounterRandom);
          ctx->battle->start(music, sd, hero, party, spec, imgmgr);
        }
      }
    }

    // 置換箇所（STATE_GAME 内）
    int cameraX = hero.X - Graphic::SCREEN_WIDTH / 2;
    int cameraY = hero.Y - Graphic::SCREEN_HEIGHT / 2;
    // map が再生成されたときは drawFullBackground を呼んでいるので通常は smartRender だけでOK
    g.smartRender(map, hero, cameraX, cameraY);


    if (ctrl.pressed(ctrl.BTN_X)) {
      menu.toggle();
      if (menu.active) state = STATE_MENU;
    }
  }

  else if (state == STATE_MENU) {
    //menu.update(ctrl, inventory, hero, party);
    menu.update(ctrl, inventory, hero, party, storage, map, ctx->session->progress(), music, sd, save, settings, vib);
    if (menu.takeSettingsSaveRequest()) {
      save.saveSettings(sd, music, settings);
    }
    //g.screenSprite.fillScreen(TFT_BLACK);
    menu.draw(&g, inventory, hero, party, storage, settings);
    //g.screenSprite.pushSprite(0,0);

    if (!menu.active) {
      state = STATE_GAME; // ゲーム画面に戻る
      //もしうまくいかなかったらここを
      ctx->requestRegenerate = false;
    }
  }

  else if (state == STATE_BATTLE) {
    auto &battle = *ctx->battle;
    battle.update(ctrl, music, sd, hero, inventory, vib, map, ctx->session->progress(), g);
    if (const std::optional<BattleResult> result = battle.takeResult()) {
      state = result->outcome == BattleOutcome::DEFEAT ? STATE_GAME_OVER : STATE_GAME;
    }
    battle.draw(&g);
    //g.screenSprite.pushSprite(0, 0);
  }

  else if (state == STATE_EVENT) {
    FieldRandom fieldRandom = {nullptr,
                               field_random_adapter::drawMax,
                               field_random_adapter::drawRange};
    FieldEventResult eventResult = ctx->fieldEventHandler.handle(
        currentEvent, map, inventory, party, storage,
        ctx->session->progress().floor(), fieldRandom);

    switch (eventResult.action) {
      case FieldEventAction::RETURN_TO_FIELD:
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::FLOOR_UP:
        if (ctx->floorManager.moveUp(ctx->session->progress())) {
          applyFloorMapConfig(ctx);
        }
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::FLOOR_DOWN:
        if (ctx->floorManager.moveDown(ctx->session->progress())) {
          applyFloorMapConfig(ctx);
        }
        state = STATE_GAME;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_INN:
        ctx->menu->active = true;
        ctx->menu->currentMenuState = Menu::STATE_INN;
        ctx->menu->listCursor = 0;
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_STORAGE:
        menu.active = true;
        menu.currentMenuState = Menu::STATE_STORAGE;
        menu.storageMode = 0;
        menu.listCursor = 0;
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::OPEN_SHOP:
        ctx->menu->active = true;
        ctx->menu->currentMenuState = Menu::STATE_SHOP_CHOICE;
        ctx->menu->listCursor = 0;
        state = STATE_MENU;
        currentEvent.type = FieldEventType::NONE;
        break;

      case FieldEventAction::START_BOSS_BATTLE: {
        int bossIndex =
            (ctx->session->progress().floor() / 10) - 1;

        if (ctx->session->progress().floor() >= 10 &&
            ctx->session->progress().floor() <= 50 &&
            ctx->session->progress().floor() % 10 == 0 &&
            bossIndex >= 0 &&
            bossIndex < 5 &&
            ctx->session->progress().bosses().isDefeated(bossIndex))
        {
          map.regenerate(
              20,
              20,
              Map::TYPE_TOWN,
              ctx->session->progress().floor()
          );
          hero.reset(map);

          music.switchTrack(0, sd);

          state = STATE_GAME;
          currentEvent.type = FieldEventType::NONE;
          break;
        }

        vib.trigger(500);

        state = STATE_BATTLE;

        if (!ctx->battle->inBattle) {
          EncounterRandom encounterRandom = {nullptr,
                                             field_random_adapter::drawMax,
                                             field_random_adapter::drawRange};
          EncounterSpec spec = encounterManager.createBossEncounter(
              ctx->session->progress().floor(), map, encounterRandom);
          ctx->battle->start(music, sd, hero, party, spec, imgmgr);
        }

        currentEvent.type = FieldEventType::NONE;
        break;
      }

      case FieldEventAction::NONE:
      default:
        state = STATE_GAME;
        break;
    }
  }

  else if (state == STATE_GAME_OVER) {
    // -------------------------------------------------
    // 1. 入力処理 (描画ループの外に出す)
    // -------------------------------------------------
    // 描画中にボタン判定をすると重くなるので、必ずループの外で行います
    if (ctrl.pressedDebounced(ctrl.BTN_A)) {
      ctx->music->playSE(0); 
      // リセット
      rp2040.reboot();
      while (1) {
        // 何もしないで死を待つ
      }
      return;
    }

    // -------------------------------------------------
    // 2. 描画処理 (ラインスプライト方式)
    // -------------------------------------------------
    // 画面上部(0)から下部(SCREEN_HEIGHT)まで、ブロック単位でループ
    for (int drawY = 0; drawY < Graphic::SCREEN_HEIGHT; drawY += Graphic::BLOCK_HEIGHT) {
      
      // (1) スプライトをクリア
      ctx->graphic->lineSprite.fillScreen(TFT_BLACK);

      // (2) 文字描画設定
      // 左上(Top-Left)を基準点にします。これがないと位置がずれることがあります。
      ctx->graphic->lineSprite.setTextDatum(TL_DATUM); 

      // (3) "GAME OVER" 描画
      ctx->graphic->lineSprite.setTextColor(TFT_RED);
      ctx->graphic->lineSprite.setTextSize(3);
      // print ではなく drawString を使います
      // Y座標: 「本来のY座標(80) - 現在の描画開始位置(drawY)」
      ctx->graphic->lineSprite.drawString("GAME OVER", 60, 80 - drawY);

      // (4) "タイトルへ" 描画
      ctx->graphic->lineSprite.setTextSize(2);
      ctx->graphic->lineSprite.setTextColor(TFT_YELLOW);
      // Y座標: 「本来のY座標(150) - 現在の描画開始位置(drawY)」
      ctx->graphic->lineSprite.drawString("タイトルへ", 80, 150 - drawY);

      // (5) 画面への転送
      // 作った短冊を、画面上の drawY の位置に貼り付けます
      ctx->graphic->lineSprite.pushSprite(0, drawY);
    }
  }

  if (prevState != state) {
    if (state == STATE_GAME) {
      // メニュー／戦闘等からゲーム画面に戻った：背景を一括再描画してヒーローを正常復帰
      int cameraX = hero.X - Graphic::SCREEN_WIDTH / 2;
      int cameraY = hero.Y - Graphic::SCREEN_HEIGHT / 2;
      g.invalidateAndRedraw(map, hero, cameraX, cameraY);
    } else {
      // メニュー／戦闘へ入るときは「画面を上書きする」ので
      // prevHero を無効化して、restore が誤動作しないようにする
      g.prevHeroScreenX = -1;
      g.prevHeroScreenY = -1;
    }
    prevState = state;
  }
}