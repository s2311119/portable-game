#include "MenuFlow.h"
#include "CharacterProgressionSystem.h"
#include "ShopTransactionSystem.h"

MenuFlowResult MenuFlow::update(Controller& ctrl, Inventory& inventory,
                                Player& hero, Party& party,
                                AutomatonStorage& storage,
                                WorldProgress& progress, Settings& settings,
                                MenuFeedbackHooks feedback) {
  if (!active_) return MenuFlowResult::NONE; // メニューが非表示なら何もしない
  bool aPressed = ctrl.pressedDebounced(ctrl.BTN_A);
  bool bPressed = ctrl.pressedDebounced(ctrl.BTN_B);
  bool upPressed = ctrl.pressedDebounced(ctrl.BTN_UP);
  bool downPressed = ctrl.pressedDebounced(ctrl.BTN_DOWN);
  bool rightPressed = ctrl.pressedDebounced(ctrl.BTN_RIGHT);
  bool leftPressed = ctrl.pressedDebounced(ctrl.BTN_LEFT);

  // --- Bボタン（キャンセル）処理 ---
  if (bPressed) {
    if (screen_ == MenuScreen::ITEM) {
      ItemMenuResult result = itemMenu.cancel();
      feedback.playSE(feedback.context, 1);
      if (result.type == ItemMenuResultType::BACK) screen_ = MenuScreen::MAIN;
      return MenuFlowResult::NONE;
    }
    if (screen_ == MenuScreen::EQUIPMENT) {
      feedback.playSE(feedback.context, 1);
      if (equipmentMenu.cancel() == EquipmentMenuResult::BACK)
        screen_ = MenuScreen::MAIN;
      return MenuFlowResult::NONE;
    }
    if (screen_ == MenuScreen::EVOLUTION) {
      feedback.playSE(feedback.context, 1);
      if (evolutionMenu.cancel() == EvolutionMenuResult::BACK)
        screen_ = MenuScreen::MAIN;
      return MenuFlowResult::NONE;
    }
    feedback.playSE(feedback.context, 1);
    if (screen_ == MenuScreen::STORAGE) {
      if (storageMenu.cancel() == StorageMenuResult::CLOSE) {
        active_ = false;
        return MenuFlowResult::CLOSE_MENU;
      }
      return MenuFlowResult::NONE;
    }else if (screen_ == MenuScreen::SHOP) {
      if (shopMenu.cancel() == ShopMenuResult::CLOSE) {
        active_ = false;
        return MenuFlowResult::CLOSE_MENU;
      }
    }else if (screen_ == MenuScreen::INN) {
      if (innMenu.cancel() == InnMenuResult::CLOSE) {
        active_ = false;
        return MenuFlowResult::CLOSE_MENU;
      }
    }else if (screen_ == MenuScreen::SKILL || screen_ == MenuScreen::SETTINGS) {
      // 進化・旋律画面でBを押したらメインに戻る
      if (screen_ == MenuScreen::SETTINGS &&
          settingsMenu.cancel() == SettingsMenuResult::BACK) {
        screen_ = MenuScreen::MAIN;
        return MenuFlowResult::SAVE_SETTINGS;
      }
      screen_ = MenuScreen::MAIN;
    } else if (screen_ == MenuScreen::MAIN) {
      // メインメニューでBを押したらメニューを閉じる
      if (mainMenu.cancel() == MainMenuResult::CLOSE) {
        active_ = false;
        return MenuFlowResult::CLOSE_MENU;
      }
    }
    return MenuFlowResult::NONE; // Bボタンが押されたフレームは他の操作を無視
  }

  // --- 状態別 Aボタン（決定）とカーソル移動 ---
  switch (screen_) {

    case MenuScreen::MAIN: {
      // --- メインメニューの操作 ---
      if (upPressed){
        mainMenu.moveUp();
        feedback.playSE(feedback.context, 2);
      }
      if (downPressed){
        mainMenu.moveDown();
        feedback.playSE(feedback.context, 2);
      }
      if (aPressed) {
        feedback.playSE(feedback.context, 0);
        switch (mainMenu.confirm()) {
          case MainMenuResult::OPEN_ITEM:
            screen_ = MenuScreen::ITEM;
            itemMenu.open();
            break;
          case MainMenuResult::OPEN_SKILL:
            screen_ = MenuScreen::SKILL;
            skillMenu.open();
            break;
          case MainMenuResult::OPEN_EVOLUTION:
            screen_ = MenuScreen::EVOLUTION;
            evolutionMenu.open();
            break;
          case MainMenuResult::OPEN_EQUIPMENT:
            screen_ = MenuScreen::EQUIPMENT;
            equipmentMenu.open();
            break;
          case MainMenuResult::REQUEST_SAVE:
            return MenuFlowResult::REQUEST_SAVE;
          case MainMenuResult::OPEN_SETTINGS:
            screen_ = MenuScreen::SETTINGS;
            settingsMenu.open();
            break;
          case MainMenuResult::NONE:
          case MainMenuResult::CLOSE:
            break;
        }
      }
      break;
    }
    case MenuScreen::ITEM: {
      if (itemMenu.state() == ItemMenu::State::ITEM_LIST) {
        int itemCount = inventory.items.size();
        if (itemCount > 0) {
          if (upPressed){
            itemMenu.moveUp(itemCount);
            feedback.playSE(feedback.context, 2);
          }
          if (downPressed){
            itemMenu.moveDown(itemCount);
            feedback.playSE(feedback.context, 2);
          }
          if (aPressed) {
            int id = inventory.items[itemMenu.cursor()].item.id;
            ItemMenuResult result = itemMenu.selectItem(itemUseSystem.canUse(id));
            feedback.playSE(feedback.context, result.type == ItemMenuResultType::REJECT ? 1 : 0);
          }
        }
      } else {
        int totalTargets = 1 + party.memberCount();
        if (upPressed){
          itemMenu.moveUp(totalTargets);
          feedback.playSE(feedback.context, 2);
        }
        if (downPressed){
          itemMenu.moveDown(totalTargets);
          feedback.playSE(feedback.context, 2);
        }
        if (aPressed) {
          ItemMenuResult result = itemMenu.requestUse();
          feedback.playSE(feedback.context, 0);
          itemUseSystem.use(result.useRequest, inventory, hero, party);
          itemMenu.completeUse();
        }
      }
      break;
    }
    case MenuScreen::STORAGE: {
      if (storageMenu.state() == StorageMenu::State::CHOICE) {
        if (upPressed || downPressed) {
          storageMenu.toggleChoice();
          feedback.playSE(feedback.context, 2);
        }
        if (aPressed) {
          storageMenu.selectChoice();
          feedback.playSE(feedback.context, 0);
        }
      }
      else if (storageMenu.state() == StorageMenu::State::WITHDRAW) {
        int sCount = storage.memberCount();
        if (sCount > 0) {
          if (upPressed) {
            storageMenu.moveUp(sCount);
            feedback.playSE(feedback.context, 2);
          }
          if (downPressed) {
            storageMenu.moveDown(sCount);
            feedback.playSE(feedback.context, 2);
          }

          if (aPressed) {
            if (party.memberCount() < Party::MAX_MEMBERS) {
              StorageMemberRequest request = storageMenu.requestMember();
              storage.withdrawMember(request.memberIndex, party);
              storageMenu.completeWithdraw();
              feedback.playSE(feedback.context, 0);
            } else {
              feedback.playSE(feedback.context, 1);
            }
          }
        }
      }
      else if (storageMenu.state() == StorageMenu::State::DEPOSIT) {
        int pCount = party.memberCount();
        if (pCount > 0) {
          if (upPressed) {
            storageMenu.moveUp(pCount);
            feedback.playSE(feedback.context, 2);
          }
          if (downPressed) {
            storageMenu.moveDown(pCount);
            feedback.playSE(feedback.context, 2);
          }
          if (aPressed) {
            StorageMemberRequest request = storageMenu.requestMember();
            party.depositMember(request.memberIndex, storage);
            storageMenu.completeDeposit(party.memberCount());
            feedback.playSE(feedback.context, 0);
          }
        }
      }
      break;
    }
    case MenuScreen::EVOLUTION: {
      int memberCount = party.memberCount();
      if (memberCount > 0) {
        if (upPressed){
          evolutionMenu.moveUp(memberCount);
          feedback.playSE(feedback.context, 2);
        }
        if (downPressed){
          evolutionMenu.moveDown(memberCount);
          feedback.playSE(feedback.context, 2);
        }

        if (aPressed) {
          EvolutionRequest request = evolutionMenu.confirm();
          const ProgressionResult evolution =
              CharacterProgressionSystem::tryItemEvolution(
                  party.memberState(request.partyIndex), inventory,
                  [](void* context) {
                    MenuFeedbackHooks& hooks =
                        *static_cast<MenuFeedbackHooks*>(context);
                    hooks.playSE(hooks.context, 1);
                  },
                  &feedback);
          const bool itemUsed = evolution.evolved;
          if (itemUsed) feedback.playSE(feedback.context, 0);

          if (itemUsed) {
            int newItemCount = inventory.items.size();
            evolutionMenu.adjustCursorAfterEvolution(memberCount);
          }
        }
      }
      break;
    }
    case MenuScreen::SKILL: {
      if (skillMenu.skillCount() > 0) {
        if (upPressed) {
          skillMenu.moveUp();
          feedback.playSE(feedback.context, 2);
        }
        if (downPressed){
          skillMenu.moveDown();
          feedback.playSE(feedback.context, 2);
        }
        if (aPressed) {
          SkillMenuResult result = skillMenu.confirm(inventory, hero);
          feedback.playSE(feedback.context, result == SkillMenuResult::LEARNED ? 0 : 1);
        }
      }
      break;
    }
    case MenuScreen::SAVE_RESULT:
      if (aPressed) {
        active_ = false;
        feedback.playSE(feedback.context, 1);
        return MenuFlowResult::CLOSE_MENU;
      }
      break;
    case MenuScreen::EQUIPMENT: {
      if (equipmentMenu.state() == EquipmentMenu::State::SLOT) {
        if (upPressed)   { equipmentMenu.moveUp(3); feedback.playSE(feedback.context, 2); }
        if (downPressed) { equipmentMenu.moveDown(3); feedback.playSE(feedback.context, 2); }
        if (aPressed) {
          feedback.playSE(feedback.context, 0);
          equipmentMenu.selectSlot();
        }
      } else {
        std::vector<int> equippableIndices = equipmentMenu.candidateIndices(inventory);
        int count = equippableIndices.size();
        if (count > 0) {
          if (upPressed)   { equipmentMenu.moveUp(count); feedback.playSE(feedback.context, 2); }
          if (downPressed) { equipmentMenu.moveDown(count); feedback.playSE(feedback.context, 2); }

          if (aPressed) {
            feedback.playSE(feedback.context, 0);
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
    case MenuScreen::INN: {
      if (upPressed || downPressed) {
        innMenu.moveSelection();
        feedback.playSE(feedback.context, 2);
      }

      if (aPressed) {
        InnMenuResult result = innMenu.confirm(inventory.money);
        if (result == InnMenuResult::STAY_REQUEST) {
          feedback.playSE(feedback.context, 0);
          inventory.money -= innMenu.cost();

          hero.state().revive(100);
          hero.state().heal(9999); hero.state().restoreTP(9999);
          for(size_t i = 0; i < party.memberCount(); ++i) {
            CharacterState& member = party.memberState(i);
            member.revive(100);
            member.heal(9999); member.restoreTP(9999);
          }
          active_ = false;
          return MenuFlowResult::CLOSE_MENU;
        } else if (result == InnMenuResult::INSUFFICIENT_FUNDS) {
          feedback.playSE(feedback.context, 1);
        } else {
          active_ = false;
          feedback.playSE(feedback.context, 1);
          return MenuFlowResult::CLOSE_MENU;
        }
      }
      break;
    }
    case MenuScreen::SHOP: {
      if (shopMenu.state() == ShopMenu::State::CHOICE) {
        if (upPressed)   { shopMenu.moveUp(3); feedback.playSE(feedback.context, 2); }
        if (downPressed) { shopMenu.moveDown(3); feedback.playSE(feedback.context, 2); }

        if (aPressed) {
          feedback.playSE(feedback.context, 0);
          if (shopMenu.selectChoice(progress.floor()) == ShopMenuResult::CLOSE) {
            active_ = false;
            return MenuFlowResult::CLOSE_MENU;
          }
        }
      } else if (shopMenu.state() == ShopMenu::State::BUY) {
        int itemCount = shopMenu.stockCount();
        if (upPressed)   { shopMenu.moveUp(itemCount); feedback.playSE(feedback.context, 2); }
        if (downPressed) { shopMenu.moveDown(itemCount); feedback.playSE(feedback.context, 2); }

        if (aPressed) {
          ShopBuyRequest request = shopMenu.requestBuy();
          ShopPurchaseQuote quote =
              ShopTransactionSystem::quotePurchase(request.itemId, inventory);

          if (quote.affordable) {
            feedback.playSE(feedback.context, 0);
            ShopTransactionSystem::completePurchase(quote, inventory);
          } else {
            feedback.playSE(feedback.context, 1);
          }
        }
      } else if (shopMenu.state() == ShopMenu::State::SELL) {
        int itemCount = inventory.items.size();
        if (itemCount > 0) {
          if (upPressed)   { shopMenu.moveUp(itemCount); feedback.playSE(feedback.context, 2); }
          if (downPressed) { shopMenu.moveDown(itemCount); feedback.playSE(feedback.context, 2); }

          if (aPressed) {
            ShopSellRequest request = shopMenu.requestSell();
            int sellPrice = ShopTransactionSystem::salePriceAt(
                inventory, request.inventoryIndex);

            feedback.playSE(feedback.context, 0);
            ShopTransactionSystem::completeSale(
                inventory, request.inventoryIndex, sellPrice);
            shopMenu.adjustCursorAfterSale(inventory.items.size());
          }
        }
      }
      break;
    }
    case MenuScreen::SETTINGS: {
      if (upPressed)   { settingsMenu.moveUp(); feedback.playSE(feedback.context, 2); }
      if (downPressed) { settingsMenu.moveDown(); feedback.playSE(feedback.context, 2); }

      if (leftPressed || rightPressed) {
        SettingsMenuChange change = settingsMenu.change(settings, rightPressed);
        if (change != SettingsMenuChange::NONE)
          feedback.settingsChanged(feedback.context, change, settings);
      }

      if (aPressed && settingsMenu.confirm() == SettingsMenuResult::BACK) {
        feedback.playSE(feedback.context, 1);
        screen_ = MenuScreen::MAIN;
        return MenuFlowResult::SAVE_SETTINGS;
      }
      break;
    }
  } // switch 終了
  return MenuFlowResult::NONE;
}
