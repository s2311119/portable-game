#pragma once

struct CoreStatsData;

class CoreStats {
public:
  int getHp() const;
  int getMaxHp() const;
  int getBaseMaxHp() const;
  int getTp() const;
  int getMaxTp() const;
  int getAttack() const;
  int getDefense() const;
  int getSpeed() const;
  int getSense() const;
  int getLuck() const;

  void initialize(const CoreStatsData& initialStats);
  void applyRecalculatedStats(const CoreStatsData& stats);
  void setHp(int val);
  int takeDamage(int damage);
  void heal(int amount);
  void revive(int hpPercent);
  bool spendTP(int cost);
  void restoreTP(int amount);
  void addAttack(int amount);
  void restoreCurrentPoints(int savedHP, int savedTP);
  void restoreToFull();
  void applyMaxHpIncrease(int increasedMaxHp);
  void restoreBaseMaxHp();

private:
  int hp = 0;
  int baseMaxHp = 0;
  int maxHp = 0;
  int tp = 0;
  int maxTp = 0;
  int attack = 0;
  int defense = 0;
  int speed = 0;
  int sense = 0;
  int luck = 0;
};
