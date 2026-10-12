#pragma once

class SkillMenu;
class Inventory;
class Player;
class Graphic;

class SkillRenderer {
public:
  void drawLine(const SkillMenu& menu, const Inventory& inventory,
                const Player& hero, Graphic& graphic, int drawY) const;
};
