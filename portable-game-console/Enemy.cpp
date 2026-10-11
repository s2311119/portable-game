#include "Enemy.h"
#include "SkillLearnsetCatalog.h"

#include <Arduino.h>
#include "EnemyCatalog.h"

namespace {

int rollLevel(int floor) {
  int level = floor + random(0, 3);
  if (level < 1) level = 1;
  return level;
}

int initialLevelFor(int requestedId, int rolledLevel) {
  if (EnemyCatalog::get(requestedId).id != requestedId) return 3;
  if (requestedId == 36) return 5;
  if (requestedId == 37) return 8;
  if (requestedId >= 80 && requestedId <= 84) return 1;
  return rolledLevel;
}

} // namespace

Enemy::Enemy(int w, int floor) : Enemy(w, rollLevel(floor), StateInitTag{}) {}

Enemy::Enemy(Enemy&&) noexcept = default;

Enemy& Enemy::operator=(Enemy&&) noexcept = default;

Enemy::Enemy(int requestedId, int rolledLevel, StateInitTag)
    : who(EnemyCatalog::get(requestedId).id),
      stateValue(initialLevelFor(requestedId, rolledLevel), who) {
  const SkillLearnsetCatalog::InitialSkills initialSkills =
      SkillLearnsetCatalog::getInitialSkills(who);
  for (int i = 0; i < initialSkills.count; i++) {
    state().forceLearnSkill(initialSkills.ids[i]);
  }
  const EnemyData& data = EnemyCatalog::get(requestedId);
  name = data.name;
  dropItemId = data.dropItemId;
  dropRatePercent = data.dropRatePercent;
  xpYield = EnemyCatalog::xpYieldFor(requestedId, rolledLevel);
}

CharacterState& Enemy::state() { return stateValue; }

const CharacterState& Enemy::state() const { return stateValue; }

Enemy::~Enemy() = default;
