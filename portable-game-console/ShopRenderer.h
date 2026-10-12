#pragma once

class ShopMenu;
class Inventory;
class Graphic;

class ShopRenderer {
public:
  void drawLine(const ShopMenu& menu, const Inventory& inventory,
                Graphic& graphic, int drawY) const;
};
