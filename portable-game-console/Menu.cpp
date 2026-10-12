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
    if (currentMenuState == STATE_EQUIP) {
      music.playSE(1);
      if (equipmentMenu.cancel() == EquipmentMenuResult::BACK)
        currentMenuState = STATE_MAIN;
      return;
    }
    if (currentMenuState == STATE_EVOLVE) {
      music.playSE(1);
      if (evolutionMenu.cancel() == EvolutionMenuResult::BACK)
        currentMenuState = STATE_MAIN;
      return;
    }
    music.playSE(1);
    if (currentMenuState == STATE_STORAGE) {
      if (storageMode != 0) storageMode = 0; // モード選択に戻る
      else active = false; // メニューを閉じる (イベント終了)
      return;
    }else if (currentMenuState == STATE_SHOP_BUY || currentMenuState == STATE_SHOP_SELL || currentMenuState == STATE_SHOP_CHOICE) {
      if (shopMenu.cancel() == ShopMenuResult::CLOSE) active = false;
      else currentMenuState = STATE_SHOP_CHOICE;
    }else if (currentMenuState == STATE_INN) {
      if (innMenu.cancel() == InnMenuResult::CLOSE) active = false;
    }else if (currentMenuState == STATE_SKILL || currentMenuState == STATE_CONFIG) {
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
            evolutionMenu.open();
            break;
          case MainMenuResult::OPEN_EQUIPMENT:
            currentMenuState = STATE_EQUIP;
            listCursor = 0;
            equipmentMenu.open();
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
      int memberCount = party.memberCount();
      if (memberCount > 0) {
        if (upPressed){
          evolutionMenu.moveUp(memberCount);
          music.playSE(2);
        }
        if (downPressed){
          evolutionMenu.moveDown(memberCount);
          music.playSE(2);
        }

        if (aPressed) {
          EvolutionRequest request = evolutionMenu.confirm();
          const ProgressionResult evolution =
              CharacterProgressionSystem::tryItemEvolution(
                  party.memberState(request.partyIndex), inventory,
                  [](void* context) {
                    static_cast<MUSIC*>(context)->playSE(1);
                  },
                  &music);
          const bool itemUsed = evolution.evolved;
          if (itemUsed) music.playSE(0);

          if (itemUsed) {
            int newItemCount = inventory.items.size();
            evolutionMenu.adjustCursorAfterEvolution(memberCount);
          }
        }
      }
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
    case STATE_EQUIP: {
      if (equipmentMenu.state() == EquipmentMenu::State::SLOT) {
        if (upPressed)   { equipmentMenu.moveUp(3); music.playSE(2); }
        if (downPressed) { equipmentMenu.moveDown(3); music.playSE(2); }
        if (aPressed) {
          music.playSE(0);
          equipmentMenu.selectSlot();
        }
      } else {
        std::vector<int> equippableIndices = equipmentMenu.candidateIndices(inventory);
        int count = equippableIndices.size();
        if (count > 0) {
          if (upPressed)   { equipmentMenu.moveUp(count); music.playSE(2); }
          if (downPressed) { equipmentMenu.moveDown(count); music.playSE(2); }

          if (aPressed) {
            music.playSE(0);
            EquipRequest request = equipmentMenu.requestEquip(equippableIndices);
            Item newItem = inventory.items[request.inventoryIndex].item;
            CharacterState* s = &hero.state();

            int oldId = -1;
            if (request.slot == EquipmentSlot::Weapon) oldId = s->equipmentState().weaponId();
            if (request.slot == EquipmentSlot::Armor) oldId = s->equipmentState().armorId();
            if (request.slot == EquipmentSlot::Accessory) oldId = s->equipmentState().accessoryId();
            if (oldId != -1) {
              inventory.addItem(ItemCatalog::get(oldId));
            }

            s->equip(request.slot, newItem.id);
            inventory.removeByIndex(request.inventoryIndex);
            equipmentMenu.completeEquip();
          }
        }
      }
      break;
    }
    case STATE_INN: {
      if (upPressed || downPressed) {
        innMenu.moveSelection();
        music.playSE(2);
      }

      if (aPressed) {
        InnMenuResult result = innMenu.confirm(inventory.money);
        if (result == InnMenuResult::STAY_REQUEST) {
          music.playSE(0);
          inventory.money -= innMenu.cost();

          hero.state().revive(100);
          hero.state().heal(9999); hero.state().restoreTP(9999);
          for(size_t i = 0; i < party.memberCount(); ++i) {
            CharacterState& member = party.memberState(i);
            member.revive(100);
            member.heal(9999); member.restoreTP(9999);
          }
          active = false;
        } else if (result == InnMenuResult::INSUFFICIENT_FUNDS) {
          music.playSE(1);
        } else {
          active = false;
          music.playSE(1);
        }
      }
      break;
    }
    case STATE_SHOP_CHOICE: {
      if (upPressed)   { shopMenu.moveUp(3); music.playSE(2); }
      if (downPressed) { shopMenu.moveDown(3); music.playSE(2); }

      if (aPressed) {
        music.playSE(0);
        if (shopMenu.selectChoice(progress.floor()) == ShopMenuResult::CLOSE) active = false;
        else if (shopMenu.state() == ShopMenu::State::BUY) currentMenuState = STATE_SHOP_BUY;
        else currentMenuState = STATE_SHOP_SELL;
      }
      break;
    }
    case STATE_SHOP_BUY: {
      int itemCount = shopMenu.stockCount();

      if (upPressed)   { shopMenu.moveUp(itemCount); music.playSE(2); }
      if (downPressed) { shopMenu.moveDown(itemCount); music.playSE(2); }

      if (aPressed) {
        ShopBuyRequest request = shopMenu.requestBuy();
        Item targetItem = ItemCatalog::get(request.itemId);

        if (inventory.money >= targetItem.price) {
          music.playSE(0);
          inventory.money -= targetItem.price;
          inventory.addItem(targetItem);
        } else {
          music.playSE(1);
        }
      }
      break;
    }
    case STATE_SHOP_SELL: {
      int itemCount = inventory.items.size();
      if (itemCount > 0) {
        if (upPressed)   { shopMenu.moveUp(itemCount); music.playSE(2); }
        if (downPressed) { shopMenu.moveDown(itemCount); music.playSE(2); }

        if (aPressed) {
          ShopSellRequest request = shopMenu.requestSell();
          Item& item = inventory.items[request.inventoryIndex].item;
          int sellPrice = item.price / 2;
          if (sellPrice < 1) sellPrice = 1;

          music.playSE(0);
          inventory.money += sellPrice;
          inventory.removeByIndex(request.inventoryIndex);
          shopMenu.adjustCursorAfterSale(inventory.items.size());
        }
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
