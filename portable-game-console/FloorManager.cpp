#include "FloorManager.h"
#include "WorldProgress.h"

bool FloorManager::moveUp(WorldProgress& progress) const {
  if (progress.currentFloor >= 50) {
    return false;
  }
  progress.currentFloor++;
  return true;
}

bool FloorManager::moveDown(WorldProgress& progress) const {
  if (progress.currentFloor <= 1) {
    return false;
  }
  progress.currentFloor--;
  return true;
}
