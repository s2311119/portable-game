#pragma once

#include "Automaton.h"

#include <cstddef>
#include <vector>

class Party;

class AutomatonStorage {
public:
  AutomatonStorage();

  bool store(Automaton&& member);
  bool withdrawMember(size_t storageIndex, Party& party);
  bool swapStoredMember(size_t storageIndex, Automaton& partyMember);

  const Automaton& member(size_t index) const;
  size_t memberCount() const;

  void clearForRestore();
  void appendRestoredMember(Automaton&& member);

private:
  std::vector<Automaton> storedMembers;
};
