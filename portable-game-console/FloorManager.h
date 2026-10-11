#pragma once

class WorldProgress;

class FloorManager {
public:
  bool moveUp(WorldProgress& progress) const;
  bool moveDown(WorldProgress& progress) const;
};
