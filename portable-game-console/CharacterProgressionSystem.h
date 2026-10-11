#pragma once

#include <vector>

class CharacterState;
class Inventory;

struct ProgressionResult {
  int oldLevel;
  int newLevel;
  bool evolved;
  int oldCharacterId;
  int newCharacterId;
  std::vector<int> learnedSkillIds;
};

class CharacterProgressionSystem {
public:
  using RuleMissCallback = void (*)(void* context);

  static ProgressionResult gainExperience(CharacterState& character, int xp);
  static ProgressionResult tryItemEvolution(
      CharacterState& character,
      Inventory& inventory,
      RuleMissCallback onRuleMiss = nullptr,
      void* context = nullptr);

private:
  friend class CharacterState;

  static void recalculateStats(CharacterState& character);
  static void learnAvailableSkills(
      CharacterState& character, int oldLevel, int newLevel);
  static void evolve(CharacterState& character, int nextId);
};
