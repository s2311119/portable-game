#include "SkillSet.h"

bool SkillSet::has(int skillId) const {
    for (int learnedId : learnedSkillIds) {
      if (learnedId == skillId) return true;
    }
    return false;
}

bool SkillSet::learn(int skillId) {
    if (has(skillId)) return false;
    learnedSkillIds.push_back(skillId);
    return true;
}

int SkillSet::count() const { return static_cast<int>(learnedSkillIds.size()); }

int SkillSet::skillIdAt(int index) const { return learnedSkillIds[index]; }

void SkillSet::reserveInitialCapacity() { learnedSkillIds.reserve(10); }

void SkillSet::clearForRestore() { learnedSkillIds.clear(); }

void SkillSet::appendRestoredSkillId(int skillId) { learnedSkillIds.push_back(skillId); }

void SkillSet::releaseStorage() {
    learnedSkillIds.clear();
    std::vector<int>().swap(learnedSkillIds);
}
