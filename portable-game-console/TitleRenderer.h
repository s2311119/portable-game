#pragma once

class Graphic;

class TitleRenderer {
public:
  void drawLine(Graphic& graphic, int titleCursor, bool hasSave, int drawY) const;
};
