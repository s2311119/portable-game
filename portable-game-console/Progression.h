#pragma once

class Progression {
public:
  void initialize(int rawLevel);
  void restore(int rawLevel, int rawXP);

  int level() const;
  int xp() const;
  int nextLevelXp() const;

  void addXP(int amount);
  bool canLevelUp() const;
  void levelUpOnce();

  static int nextLevelXpFor(int level);

private:
  int levelValue = 1;
  int xpValue = 0;
  int nextLevelXpValue = nextLevelXpFor(1);
};
