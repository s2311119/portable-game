#include "ItemMenu.h"

void ItemMenu::open() {
  state_ = State::ITEM_LIST;
  cursor_ = 0;
}

void ItemMenu::moveUp(int count) {
  cursor_ = (cursor_ - 1 + count) % count;
}

void ItemMenu::moveDown(int count) {
  cursor_ = (cursor_ + 1) % count;
}

ItemMenuResult ItemMenu::selectItem(bool usable) {
  if (!usable) {
    return {ItemMenuResultType::REJECT, {-1, -1}};
  }
  selectedItemIndex_ = cursor_;
  state_ = State::TARGET_SELECT;
  cursor_ = 0;
  return {ItemMenuResultType::NONE, {-1, -1}};
}

ItemMenuResult ItemMenu::requestUse() const {
  return {ItemMenuResultType::USE_REQUEST, {selectedItemIndex_, cursor_}};
}

ItemMenuResult ItemMenu::cancel() {
  if (state_ == State::TARGET_SELECT) {
    state_ = State::ITEM_LIST;
    return {ItemMenuResultType::NONE, {-1, -1}};
  }
  return {ItemMenuResultType::BACK, {-1, -1}};
}

void ItemMenu::completeUse() {
  state_ = State::ITEM_LIST;
  cursor_ = 0;
}

ItemMenu::State ItemMenu::state() const {
  return state_;
}

int ItemMenu::cursor() const {
  return cursor_;
}

int ItemMenu::selectedItemIndex() const {
  return selectedItemIndex_;
}
