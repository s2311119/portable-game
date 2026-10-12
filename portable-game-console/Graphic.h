#pragma once

#include "LGFX_RP2040_ILI9341.hpp"

class Graphic {
public:
  static const int SCREEN_WIDTH = 320;
  static const int SCREEN_HEIGHT = 240;
  static const int BLOCK_HEIGHT = 32;

  LGFX lcd;
  LGFX_Sprite lineSprite;

  Graphic();
  void init();
};
