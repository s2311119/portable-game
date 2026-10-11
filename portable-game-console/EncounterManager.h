#pragma once

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

private:
  int stepCounter = 0;
  const int encounterThreshold = 20;
};
