#pragma once

#include "FieldEvent.h"

class Map;
class Inventory;
class Party;
class AutomatonStorage;

struct FieldRandom {
  void* context;
  long (*randomMax)(void* context, long max);
  long (*randomRange)(void* context, long min, long max);

  long drawMax(long max) const { return randomMax(context, max); }
  long drawRange(long min, long max) const { return randomRange(context, min, max); }
};

class FieldEventHandler {
public:
  FieldEventResult handle(
      const FieldEvent& event,
      Map& map,
      Inventory& inventory,
      Party& party,
      AutomatonStorage& storage,
      int floor,
      const FieldRandom& random) const;

private:
  FieldEventResult handleTreasure(
      const FieldEvent& event,
      Map& map,
      Inventory& inventory,
      int floor,
      const FieldRandom& random) const;
  FieldEventResult handleAutomaton(
      const FieldEvent& event,
      Map& map,
      Party& party,
      AutomatonStorage& storage) const;
};
