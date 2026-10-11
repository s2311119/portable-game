#include "Party.h"
#include "Automaton.h"

#include <utility>

Party::Party() {
    activeMembers.reserve(MAX_MEMBERS); // メモリ予約
    storage.reserve(20);
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

bool Party::swapMember(int memberIdx, int storageIdx) {
    if (memberIdx < 0 || memberIdx >= activeMembers.size()) return false;
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    std::swap(activeMembers[memberIdx], *storage[storageIdx]);
    return true;
  }

bool Party::depositMember(int memberIdx) {
    if (memberIdx < 0 || memberIdx >= activeMembers.size()) return false;
    // 最後の1人は預けられない制約を入れても良い
    // if (activeMembers.size() <= 1) return false;

    storage.push_back(new Automaton(std::move(activeMembers[memberIdx])));
    activeMembers.erase(activeMembers.begin() + memberIdx);
    return true;
  }

bool Party::withdrawMember(int storageIdx) {
    if (activeMembers.size() >= MAX_MEMBERS) return false; // 満員
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    activeMembers.push_back(std::move(*storage[storageIdx]));
    delete storage[storageIdx];
    storage.erase(storage.begin() + storageIdx);
    return true;
  }

Party::~Party() {
    for (auto* s : storage) delete s;
    storage.clear();
  }
