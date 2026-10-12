#include "AutomatonStorage.h"
#include "Party.h"

#include <utility>

AutomatonStorage::AutomatonStorage() { storedMembers.reserve(20); }

bool AutomatonStorage::store(Automaton&& member) {
  storedMembers.push_back(std::move(member));
  return true;
}

bool AutomatonStorage::withdrawMember(size_t storageIndex, Party& party) {
  if (storageIndex >= storedMembers.size()) return false;
  if (!party.addMember(std::move(storedMembers[storageIndex]))) return false;
  storedMembers.erase(storedMembers.begin() + storageIndex);
  return true;
}

bool AutomatonStorage::swapStoredMember(size_t storageIndex, Automaton& partyMember) {
  if (storageIndex >= storedMembers.size()) return false;
  std::swap(storedMembers[storageIndex], partyMember);
  return true;
}

const Automaton& AutomatonStorage::member(size_t index) const {
  return storedMembers[index];
}

size_t AutomatonStorage::memberCount() const { return storedMembers.size(); }

void AutomatonStorage::clearForRestore() { storedMembers.clear(); }

void AutomatonStorage::appendRestoredMember(Automaton&& member) {
  storedMembers.push_back(std::move(member));
}
