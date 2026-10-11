#pragma once

enum class FieldEventType {
  NONE,
  TREASURE,
  STAIR_UP,
  STAIR_DOWN,
  AUTOMATON,
  INN,
  SHOP,
  STORAGE,
  BOSS_BATTLE
};

struct FieldEvent {
  FieldEventType type;
  int tileX;
  int tileY;
};
