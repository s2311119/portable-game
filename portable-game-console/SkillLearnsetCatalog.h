#pragma once

struct SkillLearningRule {
  int characterId;
  int level;
  int skillId;
};

class SkillLearnsetCatalog {
public:
  static constexpr int LEARNING_RULE_COUNT = 108;
  static constexpr int INITIAL_SKILL_COUNT = 26;

  struct LearnableSkills {
    int ids[LEARNING_RULE_COUNT];
    int count;
  };

  struct InitialSkills {
    int ids[INITIAL_SKILL_COUNT];
    int count;
  };

  static LearnableSkills getLearnableSkills(int characterId, int oldLevel, int newLevel);
  static InitialSkills getInitialSkills(int characterId);
};
