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

FloorMapConfig FloorManager::getConfig(const WorldProgress& progress) const {
  const int floor = progress.floor();
  // 1Fは集落
  if (floor == 1) {
    return {
      0,
      0,
      Map::TYPE_TOWN,
      0
    };
  }

  // 10F / 20F / 30F / 40F / 50F
  if (floor % 10 == 0) {
    int bossIndex = (floor / 10) - 1;

    if (bossIndex >= 0 && bossIndex < 5) {
      if (progress.bosses().isDefeated(bossIndex)) {
        return {
          5,
          5,
          Map::TYPE_TOWN,
          0
        };
      }

      return {
        5,
        5,
        Map::TYPE_BOSS_ROOM,
        1
      };
    }
  }

  // 通常階
  return {
    20,
    20,
    Map::TYPE_DUNGEON,
    1
  };
}
