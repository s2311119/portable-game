#pragma once

enum class ItemMenuResultType {
  NONE,
  BACK,
  USE_REQUEST,
  REJECT
};

struct ItemUseRequest {
  int itemIndex;
  int targetIndex;
};

struct ItemMenuResult {
  ItemMenuResultType type;
  ItemUseRequest useRequest;
};

class ItemMenu {
public:
  enum class State {
    ITEM_LIST,
    TARGET_SELECT
  };

private:
  State state_ = State::ITEM_LIST;
  int cursor_ = 0;
  int selectedItemIndex_ = -1;

public:
  void open();
  void moveUp(int count);
  void moveDown(int count);
  ItemMenuResult selectItem(bool usable);
  ItemMenuResult requestUse() const;
  ItemMenuResult cancel();
  void completeUse();

  State state() const;
  int cursor() const;
  int selectedItemIndex() const;
};
