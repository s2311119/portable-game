#pragma once

class EquipmentMenu;
class Inventory;
class Player;
class Graphic;

class EquipmentRenderer {
public:
  void drawLine(const EquipmentMenu& menu, const Inventory& inventory,
                const Player& player, Graphic& graphic, int drawY) const;
};
