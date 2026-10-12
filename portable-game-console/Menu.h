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

class Menu {
public:
  // ★ メニューの状態を定義
  enum MenuState {
    STATE_MAIN,     // 道具、技、進化... を選ぶ
    STATE_EVOLVE,    // ★ 「進化」画面
    STATE_SKILL,
    STATE_ITEM,
    STATE_STORAGE, // ★ 預かり所画面
    STATE_SAVE_RESULT,
    // --- 装備用 ---
    STATE_EQUIP,
    // ショップ・宿屋
    STATE_INN,          // 宿屋 (泊まりますか？)
    STATE_SHOP_CHOICE,  // 店 (買う/売る/出る)
    STATE_SHOP_BUY,     // 買う画面
    STATE_SHOP_SELL,     // 売る画面
    STATE_CONFIG
  };
  MenuState currentMenuState;

  // 進化画面
  int listCursor = 0;
  bool active = false;
  bool saveSucceeded = false;
  std::vector<int> shopItemList;
  int storageMode = 0; // 0:メニュー選択, 1:引き出す, 2:預ける

  Menu() {
    currentMenuState = STATE_MAIN; // 初期状態
  }

  void toggle() {
    active = !active;
    currentMenuState = STATE_MAIN; // メニューを開くときは必ずメインから
    mainMenu.reset();
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
        mainMenuRenderer.drawLine(mainMenu, *g, drawY);

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
                  uint16_t color = (i == evolutionMenu.cursor()) ? TFT_YELLOW : TFT_WHITE;
                  g->lineSprite.setTextColor(color);
                  g->lineSprite.setCursor(x + 20, y + 55 + i * 28);
                  g->lineSprite.printf("%s Lv%d HP%d/%d", ally.state().name(), ally.state().progress().level(), ally.state().stats().getHp(), ally.state().stats().getMaxHp());
              }
          }

      } else if (currentMenuState == STATE_SKILL) {
        skillRenderer.drawLine(skillMenu, inventory, hero, *g, drawY);

      } else if (currentMenuState == STATE_ITEM) {
        itemRenderer.drawLine(itemMenu, inventory, hero, party, *g, drawY);

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
      }else if (currentMenuState == STATE_EQUIP) {
        equipmentRenderer.drawLine(equipmentMenu, inventory, hero, *g, drawY);
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
  bool settingsSaveRequested = false;
};
