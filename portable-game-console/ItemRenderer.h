#pragma once

class ItemMenu;
class Inventory;
class Player;
class Party;
class Graphic;

class ItemRenderer {
public:
  void drawLine(const ItemMenu& menu, const Inventory& inventory,
                const Player& hero, const Party& party, Graphic& graphic,
                int drawY) const;
};
