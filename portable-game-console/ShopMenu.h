#pragma once

#include <vector>

struct ShopBuyRequest {
  int itemId;
};

struct ShopSellRequest {
  int inventoryIndex;
};

enum class ShopMenuResult {
  NONE,
  CLOSE
};

class ShopMenu {
public:
  enum class State {
    CHOICE,
    BUY,
    SELL
  };

  void open();
  void moveUp(int itemCount);
  void moveDown(int itemCount);
  ShopMenuResult selectChoice(int floor);
  ShopMenuResult cancel();
  ShopBuyRequest requestBuy() const;
  ShopSellRequest requestSell() const;
  void adjustCursorAfterSale(int itemCount);

  State state() const { return state_; }
  int cursor() const { return cursor_; }
  int stockCount() const { return stockItemIds_.size(); }
  int stockItemIdAt(int index) const { return stockItemIds_[index]; }

private:
  State state_ = State::CHOICE;
  int cursor_ = 0;
  std::vector<int> stockItemIds_;
};
