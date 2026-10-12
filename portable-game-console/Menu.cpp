#include "Menu.h"
#include "CharacterProgressionSystem.h"

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
    if (currentMenuState == STATE_ITEM) {
      ItemMenuResult result = itemMenu.cancel();
      music.playSE(1);
      if (result.type == ItemMenuResultType::BACK) currentMenuState = STATE_MAIN;
      return;
    }
    music.playSE(1);
    if (currentMenuState == STATE_STORAGE) {
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
    }else if (currentMenuState == STATE_EQUIP_SLOT||currentMenuState == STATE_EVOLVE || currentMenuState == STATE_SKILL || currentMenuState == STATE_CONFIG) {
      // 進化・旋律画面でBを押したらメインに戻る
      if (currentMenuState == STATE_CONFIG) settingsSaveRequested = true;
      currentMenuState = STATE_MAIN;
    } else if (currentMenuState == STATE_MAIN) {
      // メインメニューでBを押したらメニューを閉じる
      if (mainMenu.cancel() == MainMenuResult::CLOSE) active = false;
    }
    return; // Bボタンが押されたフレームは他の操作を無視
  }

  // --- 状態別 Aボタン（決定）とカーソル移動 ---
  switch (currentMenuState) {

    case STATE_MAIN: {
      // --- メインメニューの操作 ---
      if (upPressed){
        mainMenu.moveUp();
        music.playSE(2);
      }
      if (downPressed){
        mainMenu.moveDown();
        music.playSE(2);
      }
      if (aPressed) {
        music.playSE(0);
        switch (mainMenu.confirm()) {
          case MainMenuResult::OPEN_ITEM:
            currentMenuState = STATE_ITEM;
            listCursor = 0;
            itemMenu.open();
            break;
          case MainMenuResult::OPEN_SKILL:
            currentMenuState = STATE_SKILL;
            listCursor = 0;
            skillMenu.open();
            break;
          case MainMenuResult::OPEN_EVOLUTION:
            currentMenuState = STATE_EVOLVE;
            listCursor = 0;
            break;
          case MainMenuResult::OPEN_EQUIPMENT:
            currentMenuState = STATE_EQUIP_SLOT;
            listCursor = 0;
            break;
          case MainMenuResult::REQUEST_SAVE:
            saveSucceeded = saveMgr.saveGame(
                sd, music, map, progress, hero, party, storage, inventory);
            currentMenuState = STATE_SAVE_RESULT;
            break;
          case MainMenuResult::OPEN_SETTINGS:
            currentMenuState = STATE_CONFIG;
            listCursor = 0;
            break;
          case MainMenuResult::NONE:
          case MainMenuResult::CLOSE:
            break;
        }
      }
      break;
    }
    case STATE_ITEM: {
      if (itemMenu.state() == ItemMenu::State::ITEM_LIST) {
        int itemCount = inventory.items.size();
        if (itemCount > 0) {
          if (upPressed){
            itemMenu.moveUp(itemCount);
            music.playSE(2);
          }
          if (downPressed){
            itemMenu.moveDown(itemCount);
            music.playSE(2);
          }
          if (aPressed) {
            int id = inventory.items[itemMenu.cursor()].item.id;
            ItemMenuResult result = itemMenu.selectItem(itemUseSystem.canUse(id));
            music.playSE(result.type == ItemMenuResultType::REJECT ? 1 : 0);
          }
        }
      } else {
        int totalTargets = 1 + party.memberCount();
        if (upPressed){
          itemMenu.moveUp(totalTargets);
          music.playSE(2);
        }
        if (downPressed){
          itemMenu.moveDown(totalTargets);
          music.playSE(2);
        }
        if (aPressed) {
          ItemMenuResult result = itemMenu.requestUse();
          music.playSE(0);
          itemUseSystem.use(result.useRequest, inventory, hero, party);
          itemMenu.completeUse();
        }
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
      if (skillMenu.skillCount() > 0) {
        if (upPressed) {
          skillMenu.moveUp();
          music.playSE(2);
        }
        if (downPressed){
          skillMenu.moveDown();
          music.playSE(2);
        }
        if (aPressed) {
          SkillMenuResult result = skillMenu.confirm(inventory, hero);
          music.playSE(result == SkillMenuResult::LEARNED ? 0 : 1);
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
