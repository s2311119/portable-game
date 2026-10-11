#pragma once

#include <vector>

class SkillSet {
public:
  bool has(int skillId) const;
  bool learn(int skillId);
  int count() const;
  int skillIdAt(int index) const;

  void reserveInitialCapacity();
  void clearForRestore();
  void appendRestoredSkillId(int skillId);
  void releaseStorage();

private:
  std::vector<int> learnedSkillIds;
};
