#pragma once

class InnMenu;
class Graphic;

class InnRenderer {
public:
  void drawLine(const InnMenu& menu, Graphic& graphic, int drawY) const;
};
