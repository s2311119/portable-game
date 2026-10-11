#include "CharacterProgressionSystem.h"
#include "CharacterState.h"
#include "CharacterCatalog.h"
#include "EvolutionCatalog.h"
#include "EvolutionSystem.h"
#include "Inventory.h"
#include "SkillLearnsetCatalog.h"

namespace {

int normalizedSkillCharacterId(int id) {
  if (id >= 0 && id <= 37) return id + 39;
  return id;
}

int previousEvolutionId(int currentId) {
  const EvolutionCatalog::Lineage lineage = EvolutionCatalog::getLineage(currentId);
  return lineage.count > 1 ? lineage.ids[1] : -1;
}

ProgressionResult initialResult(const CharacterState& character) {
  return {character.progress().level(), character.progress().level(), false,
          character.id(), character.id(), {}};
}

void finishResult(ProgressionResult& result,
                  const CharacterState& character,
                  int previousSkillCount) {
  result.newLevel = character.progress().level();
  result.newCharacterId = character.id();
  const SkillSet& skills = character.skills();
  for (int i = previousSkillCount; i < skills.count(); i++) {
    result.learnedSkillIds.push_back(skills.skillIdAt(i));
  }
}

} // namespace

void CharacterProgressionSystem::recalculateStats(CharacterState& character) {
  const CoreStatsData stats =
      CharacterCatalog::statsFor(character.who, character.progression.level());
  character.coreStats.applyRecalculatedStats(stats);
}

void CharacterProgressionSystem::learnAvailableSkills(
    CharacterState& character, int oldLevel, int newLevel) {
  int checkID = character.who;
  while (checkID != -1) {
    const int normalizedCheckID = normalizedSkillCharacterId(checkID);
    const SkillLearnsetCatalog::LearnableSkills skills =
        SkillLearnsetCatalog::getLearnableSkills(
            normalizedCheckID, oldLevel, newLevel);
    for (int i = 0; i < skills.count; i++) {
      character.learnSkill(skills.ids[i]);
    }
    checkID = previousEvolutionId(checkID);
  }
}

void CharacterProgressionSystem::evolve(CharacterState& character, int nextId) {
  const int oldLevel = character.progression.level();
  EvolutionSystem::applyIdentity(character, nextId);
  recalculateStats(character);
  character.coreStats.restoreToFull();
  learnAvailableSkills(character, 0, oldLevel);
}

ProgressionResult CharacterProgressionSystem::gainExperience(
    CharacterState& character, int xp) {
  ProgressionResult result = initialResult(character);
  const int previousSkillCount = character.skillSet.count();
  if (character.progression.level() >= 99) {
    return result;
  }

  character.progression.addXP(xp);
  while (character.progression.canLevelUp()) {
    const int previousLevel = character.progression.level();
    character.progression.levelUpOnce();

    bool evolvedAtThisLevel = false;
    while (true) {
      const int nextId = EvolutionSystem::nextLevelEvolutionId(character);
      if (nextId == -1) break;
      evolve(character, nextId);
      result.evolved = true;
      evolvedAtThisLevel = true;
    }
    if (!evolvedAtThisLevel) {
      recalculateStats(character);
      learnAvailableSkills(character, previousLevel, character.progression.level());
    }
  }

  finishResult(result, character, previousSkillCount);
  return result;
}

ProgressionResult CharacterProgressionSystem::tryItemEvolution(
    CharacterState& character,
    Inventory& inventory,
    RuleMissCallback onRuleMiss,
    void* context) {
  ProgressionResult result = initialResult(character);
  const int previousSkillCount = character.skillSet.count();
  const EvolutionCatalog::RuleView rules = EvolutionCatalog::getRules();

  for (int i = 0; i < rules.count; i++) {
    const EvolutionCatalog::EvolutionRule& rule = rules.rules[i];
    if (rule.before_who_id == character.id() &&
        rule.trigger_type == EvolutionCatalog::TRIGGER_ITEM) {
      const int requiredItemId = rule.trigger_value;
      int foundIndex = -1;
      for (int j = 0; j < inventory.items.size(); j++) {
        if (inventory.items[j].item.id == requiredItemId) {
          foundIndex = j;
          break;
        }
      }
      if (foundIndex != -1) {
        evolve(character, rule.after_who_id);
        inventory.removeByIndex(foundIndex);
        result.evolved = true;
        break;
      }
    }
    if (onRuleMiss != nullptr) onRuleMiss(context);
  }

  finishResult(result, character, previousSkillCount);
  return result;
}
