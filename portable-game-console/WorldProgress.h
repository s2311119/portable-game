#pragma once

class FloorManager;

class BossProgress {
public:
  bool isDefeated(int bossIndex) const;
  void markDefeated(int bossIndex);
  void restoreDefeated(int bossIndex, bool value);

private:
  bool defeated[5] = {};
};

class WorldProgress {
public:
  int floor() const;
  void restoreFloor(int rawFloor);

  const BossProgress& bosses() const;
  void markBossDefeated(int bossIndex);
  void restoreBossDefeated(int bossIndex, bool value);

private:
  friend class FloorManager;
  int currentFloor = 1;
  BossProgress bossProgress;
};
