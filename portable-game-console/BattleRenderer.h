#pragma once

class Battle;
class Player;
class Party;
class ImageManager;
class Graphic;

class BattleRenderer {
public:
  void draw(const Battle& battle, const Player& hero, const Party& party,
            const ImageManager& images, Graphic& graphic) const;
};
