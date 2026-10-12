#pragma once
#include "Map.h"

class WorldProgress;

struct FloorMapConfig {
  int width;
  int height;
  Map::MapType type;
};

class FloorManager {
public:
  bool moveUp(WorldProgress& progress) const;
  bool moveDown(WorldProgress& progress) const;
  FloorMapConfig getConfig(const WorldProgress& progress) const;
};
