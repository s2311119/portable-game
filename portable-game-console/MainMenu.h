#pragma once

enum class MainMenuResult {
  NONE,
  OPEN_ITEM,
  OPEN_SKILL,
  OPEN_EVOLUTION,
  OPEN_EQUIPMENT,
  REQUEST_SAVE,
  OPEN_SETTINGS,
  CLOSE
};

class MainMenu {
private:
  static const int ITEM_COUNT = 6;
  const char* items[ITEM_COUNT] = {"道具", "旋律", "進化", "装備", "記録", "設定"};
  int selected = 0;

public:
  void reset();
  void moveUp();
  void moveDown();
  MainMenuResult confirm() const;
  MainMenuResult cancel() const;

  int itemCount() const;
  int selectedIndex() const;
  const char* itemLabelAt(int index) const;
};
