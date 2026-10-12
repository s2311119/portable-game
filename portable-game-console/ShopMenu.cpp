#include "ShopMenu.h"

void ShopMenu::open() {
  state_ = State::CHOICE;
  cursor_ = 0;
}

void ShopMenu::moveUp(int itemCount) {
  cursor_ = (cursor_ - 1 + itemCount) % itemCount;
}

void ShopMenu::moveDown(int itemCount) {
  cursor_ = (cursor_ + 1) % itemCount;
}

ShopMenuResult ShopMenu::selectChoice(int floor) {
  if (cursor_ == 0) {
    state_ = State::BUY;
    cursor_ = 0;

    stockItemIds_.clear();
    stockItemIds_.push_back(100);
    stockItemIds_.push_back(103);
    stockItemIds_.push_back(104);
    if (floor >= 10) stockItemIds_.push_back(101);

    int tier = (floor - 1) / 10;
    if (tier > 4) tier = 4;
    stockItemIds_.push_back(300 + tier);
    stockItemIds_.push_back(400 + tier);
    if (tier >= 1) stockItemIds_.push_back(500);
    return ShopMenuResult::NONE;
  }
  if (cursor_ == 1) {
    state_ = State::SELL;
    cursor_ = 0;
    return ShopMenuResult::NONE;
  }
  return ShopMenuResult::CLOSE;
}

ShopMenuResult ShopMenu::cancel() {
  if (state_ == State::CHOICE) return ShopMenuResult::CLOSE;
  state_ = State::CHOICE;
  // The shared cursor remains at the last buy/sell position on B.
  return ShopMenuResult::NONE;
}

ShopBuyRequest ShopMenu::requestBuy() const {
  return {stockItemIds_[cursor_]};
}

ShopSellRequest ShopMenu::requestSell() const {
  return {cursor_};
}

void ShopMenu::adjustCursorAfterSale(int itemCount) {
  if (cursor_ >= itemCount) cursor_ = itemCount > 0 ? itemCount - 1 : 0;
}
