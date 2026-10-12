#pragma once

class MainMenu;
class Graphic;

class MainMenuRenderer {
public:
  void drawLine(const MainMenu& menu, Graphic& graphic, int drawY) const;
};
