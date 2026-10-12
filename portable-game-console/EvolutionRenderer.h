#pragma once

class EvolutionMenu;
class Party;
class Graphic;

class EvolutionRenderer {
public:
  void drawLine(const EvolutionMenu& menu, const Party& party,
                Graphic& graphic, int drawY) const;
};
