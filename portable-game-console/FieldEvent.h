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

enum class FieldEventAction {
  NONE,
  RETURN_TO_FIELD,
  FLOOR_UP,
  FLOOR_DOWN,
  OPEN_SHOP,
  OPEN_INN,
  OPEN_STORAGE,
  START_BOSS_BATTLE
};

struct FieldEventResult {
  FieldEventAction action;
};
