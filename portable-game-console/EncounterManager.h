#pragma once

#include "EncounterSpec.h"

class Map;

struct EncounterRandom {
  void* context;
  long (*randomMax)(void* context, long max);
  long (*randomRange)(void* context, long min, long max);

  long drawMax(long max) const { return randomMax(context, max); }
  long drawRange(long min, long max) const { return randomRange(context, min, max); }
};

class EncounterManager {
public:
  bool onStep(const Map& map, const EncounterRandom& random);
  EncounterSpec createEncounter(int floor, const Map& map,
                                const EncounterRandom& random) const;
  EncounterSpec createBossEncounter(int floor, const Map& map,
                                    const EncounterRandom& random) const;

private:
  EncounterSpec buildBossEncounter(int floor,
                                   const EncounterRandom& random) const;
  EncounterSpec createNormalEncounter(int floor,
                                      const EncounterRandom& random) const;
  static int rollLevel(int floor, const EncounterRandom& random);

  int stepCounter = 0;
  const int encounterThreshold = 20;
};
