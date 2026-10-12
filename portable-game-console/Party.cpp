#include "Party.h"
#include "Automaton.h"
#include "AutomatonStorage.h"

#include <utility>

Party::Party() {
    activeMembers.reserve(MAX_MEMBERS); // メモリ予約
  }

bool Party::addMember(Automaton&& newMember) {
    if (activeMembers.size() >= MAX_MEMBERS) return false;
    activeMembers.push_back(std::move(newMember));
    return true;
  }

void Party::appendRestoredMember(Automaton&& restoredMember) {
    activeMembers.push_back(std::move(restoredMember));
}

void Party::clearMembers() { activeMembers.clear(); }

const Automaton& Party::member(size_t index) const { return activeMembers[index]; }

CharacterState& Party::memberState(size_t index) { return activeMembers[index].state(); }

size_t Party::memberCount() const { return activeMembers.size(); }

bool Party::swapMember(size_t memberIdx, AutomatonStorage& storage, size_t storageIdx) {
    if (memberIdx >= activeMembers.size()) return false;
    return storage.swapStoredMember(storageIdx, activeMembers[memberIdx]);
  }

bool Party::depositMember(size_t memberIdx, AutomatonStorage& storage) {
    if (memberIdx >= activeMembers.size()) return false;
    // 最後の1人は預けられない制約を入れても良い
    // if (activeMembers.size() <= 1) return false;

    if (!storage.store(std::move(activeMembers[memberIdx]))) return false;
    activeMembers.erase(activeMembers.begin() + memberIdx);
    return true;
  }

Party::~Party() = default;
