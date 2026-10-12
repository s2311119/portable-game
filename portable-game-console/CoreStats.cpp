#include "CoreStats.h"
#include "CharacterCatalog.h"

int CoreStats::getHp() const { return hp; }

int CoreStats::getMaxHp() const { return maxHp; }

int CoreStats::getBaseMaxHp() const { return baseMaxHp; }

int CoreStats::getTp() const { return tp; }

int CoreStats::getMaxTp() const { return maxTp; }

int CoreStats::getAttack() const { return attack; }

int CoreStats::getDefense() const { return defense; }

int CoreStats::getSpeed() const { return speed; }

int CoreStats::getSense() const { return sense; }

int CoreStats::getLuck() const { return luck; }

void CoreStats::initialize(const CoreStatsData& initialStats) {
    maxHp = initialStats.maxHp;
    baseMaxHp = maxHp;
    hp = maxHp;
    //tune point
    maxTp = initialStats.maxTp;
    tp = maxTp;
    //物理攻撃に関係
    attack = initialStats.attack;
    //防御に関係
    defense = initialStats.defense;
    //回避率、命中率に関係
    speed = initialStats.speed;
    //旋律、ノイズ攻撃に使用
    sense = initialStats.sense;
    // ドロ率、クリ率に関係
    luck = initialStats.luck;
}

void CoreStats::applyRecalculatedStats(const CoreStatsData& stats) {

    // 現在のHPが最大HPを超えるのを防ぐため、上昇分だけ加算する
    int oldMaxHP = maxHp;
    maxHp = stats.maxHp;
    baseMaxHp = maxHp;
    hp += (maxHp - oldMaxHP); // 上昇分を加算
    if (hp > maxHp) hp = maxHp; // (もし全回復させたいなら HP = MaxHP;)

    // TPや他のステータスは再計算
    maxTp = stats.maxTp;
    tp = maxTp; // TPは全回復
    attack = stats.attack;
    defense = stats.defense;
    speed = stats.speed;
    sense = stats.sense;
    luck = stats.luck;
}

void CoreStats::setHp(int val) { hp = val;}

int CoreStats::takeDamage(int damage) {
    hp -= damage;
    if(hp < 0) hp = 0;
    return damage;
}

void CoreStats::heal(int amount) {
    if (hp <= 0) return; // 戦闘不能者は回復しない (蘇生は Revive を使う)
    
    hp += amount;
    if (hp > maxHp) {
        hp = maxHp;
    }
  }

void CoreStats::revive(int hpPercent) {
    if (hp > 0) return; // 既に生きている

    if (hpPercent <= 0) hpPercent = 1;
    if (hpPercent > 100) hpPercent = 100;
    
    // 最大HPの N% で復活
    hp = maxHp * hpPercent / 100;
  }

bool CoreStats::spendTP(int cost) {
    if (tp >= cost) {
      tp -= cost;
      return true;
    }
    return false; // TPが足りない
  }

void CoreStats::restoreTP(int amount) {
    tp += amount;
    if (tp > maxTp) {
        tp = maxTp;
    }
    if (tp < 0) {
        tp = 0;
    }
  }

void CoreStats::addAttack(int amount) {
    attack += amount;
    // (将来的に上限なども設定)
  }

void CoreStats::restoreCurrentPoints(int savedHP, int savedTP) {
    hp = savedHP;
    tp = savedTP;

    // 壊れた/古いセーブデータ対策
    if (hp < 0) hp = 0;
    if (hp > maxHp) hp = maxHp;

    if (tp < 0) tp = 0;
    if (tp > maxTp) tp = maxTp;
}

void CoreStats::restoreToFull() {
    this->hp = this->maxHp;
    this->tp = this->maxTp;
}

void CoreStats::applyMaxHpIncrease(int increasedMaxHp) {
    maxHp = increasedMaxHp;
}

void CoreStats::restoreBaseMaxHp() {
    maxHp = baseMaxHp; // HP上限を戻す
    if (hp > maxHp) hp = maxHp;
}
