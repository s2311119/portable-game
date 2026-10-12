#include "EvolutionSystem.h"
#include "CharacterState.h"
#include "EvolutionCatalog.h"

int EvolutionSystem::nextLevelEvolutionId(const CharacterState& character) {
  const EvolutionCatalog::RuleView rules = EvolutionCatalog::getRules();
  for (int i = 0; i < rules.count; i++) {
    const EvolutionCatalog::EvolutionRule& rule = rules.rules[i];
    if (rule.before_who_id == character.id() &&
        rule.trigger_type == EvolutionCatalog::TRIGGER_LEVEL &&
        character.progress().level() >= rule.trigger_value) {
      return rule.after_who_id;
    }
  }
  return -1;
}

void EvolutionSystem::applyIdentity(CharacterState& character, int nextId) {
  character.who = nextId;
  character.updateName();
}
