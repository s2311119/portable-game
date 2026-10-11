#pragma once

#include <stdint.h>

class Player;
class Map;

enum class FieldDirection {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

struct ExplorationInput {
  void* context;
  bool (*pressed)(void* context, FieldDirection direction);

  bool isPressed(FieldDirection direction) const {
    return pressed(context, direction);
  }
};

class Exploration {
public:
  void updateMovement(
      Player& hero,
      Map& map,
      uint32_t nowMs,
      const ExplorationInput& input);

private:
  uint32_t lastMoveTime = 0;
  const uint32_t moveIntervalMs = 80;

  void move(Player& hero, uint32_t nowMs, const ExplorationInput& input);
  void checkWall(Player& hero, Map& map);
};
