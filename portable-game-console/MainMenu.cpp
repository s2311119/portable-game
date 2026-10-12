#include "MainMenu.h"

void MainMenu::reset() {
  selected = 0;
}

void MainMenu::moveUp() {
  selected = (selected - 1 + ITEM_COUNT) % ITEM_COUNT;
}

void MainMenu::moveDown() {
  selected = (selected + 1) % ITEM_COUNT;
}

MainMenuResult MainMenu::confirm() const {
  switch (selected) {
    case 0: return MainMenuResult::OPEN_ITEM;
    case 1: return MainMenuResult::OPEN_SKILL;
    case 2: return MainMenuResult::OPEN_EVOLUTION;
    case 3: return MainMenuResult::OPEN_EQUIPMENT;
    case 4: return MainMenuResult::REQUEST_SAVE;
    case 5: return MainMenuResult::OPEN_SETTINGS;
    default: return MainMenuResult::NONE;
  }
}

MainMenuResult MainMenu::cancel() const {
  return MainMenuResult::CLOSE;
}

int MainMenu::itemCount() const {
  return ITEM_COUNT;
}

int MainMenu::selectedIndex() const {
  return selected;
}

const char* MainMenu::itemLabelAt(int index) const {
  return items[index];
}
