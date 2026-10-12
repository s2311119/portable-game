#pragma once

class Inventory;
class Player;

enum class SkillMenuResult {
  LEARNED,
  REJECT
};

class SkillMenu {
private:
  static const int learnableSkillIds_[];
  int cursor_ = 0;

public:
  void open();
  void moveUp();
  void moveDown();
  SkillMenuResult confirm(Inventory& inventory, Player& hero);

  int cursor() const;
  int skillCount() const;
  int skillIdAt(int index) const;
};
