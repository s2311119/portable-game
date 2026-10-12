#include "WorldProgress.h"

bool BossProgress::isDefeated(int bossIndex) const {
  return defeated[bossIndex];
}

void BossProgress::markDefeated(int bossIndex) {
  defeated[bossIndex] = true;
}

void BossProgress::restoreDefeated(int bossIndex, bool value) {
  defeated[bossIndex] = value;
}

int WorldProgress::floor() const {
  return currentFloor;
}

void WorldProgress::restoreFloor(int rawFloor) {
  currentFloor = rawFloor;
}

const BossProgress& WorldProgress::bosses() const {
  return bossProgress;
}

void WorldProgress::markBossDefeated(int bossIndex) {
  bossProgress.markDefeated(bossIndex);
}

void WorldProgress::restoreBossDefeated(int bossIndex, bool value) {
  bossProgress.restoreDefeated(bossIndex, value);
}
