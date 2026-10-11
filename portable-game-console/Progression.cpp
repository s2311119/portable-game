#include "Progression.h"

void Progression::initialize(int rawLevel) {
    levelValue = rawLevel;
    xpValue = 0;
    nextLevelXpValue = nextLevelXpFor(levelValue);
}

void Progression::restore(int rawLevel, int rawXP) {
    levelValue = rawLevel;
    xpValue = rawXP;
    nextLevelXpValue = nextLevelXpFor(levelValue);
}

int Progression::level() const { return levelValue; }

int Progression::xp() const { return xpValue; }

int Progression::nextLevelXp() const { return nextLevelXpValue; }

void Progression::addXP(int amount) {
    xpValue += amount;
}

bool Progression::canLevelUp() const {
    return xpValue >= nextLevelXpValue && levelValue < 99;
}

void Progression::levelUpOnce() {
    xpValue -= nextLevelXpValue;
    ++levelValue;
    nextLevelXpValue = nextLevelXpFor(levelValue);
}

int Progression::nextLevelXpFor(int level) {
    return 10 + (level * level * 5);
  }
