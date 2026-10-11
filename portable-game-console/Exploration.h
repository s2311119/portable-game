#pragma once

#include <stdint.h>
#include "FieldEvent.h"

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

struct ExplorationResult {
  bool moved;
  bool hasEvent;
  FieldEvent event;
};

class Exploration {
public:
  void updateMovement(
      Player& hero,
      Map& map,
      uint32_t nowMs,
      const ExplorationInput& input);
  ExplorationResult completeStep(
      Player& hero,
      Map& map,
      int previousX,
      int previousY,
      bool actionPressed);

private:
  uint32_t lastMoveTime = 0;
  const uint32_t moveIntervalMs = 80;

  void move(Player& hero, uint32_t nowMs, const ExplorationInput& input);
  void checkWall(Player& hero, Map& map);
  FieldEvent detectEvent(Map& map, int tileX, int tileY) const;
};
