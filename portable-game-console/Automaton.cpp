#include "Automaton.h"
#include "CharacterState.h"

Automaton::Automaton(int level, int w) : stateValue(level, w) {}

Automaton::Automaton(Automaton&&) noexcept = default;

Automaton& Automaton::operator=(Automaton&&) noexcept = default;

CharacterState& Automaton::state() { return stateValue; }

const CharacterState& Automaton::state() const { return stateValue; }

const char* Automaton::getname() {
    return state().name();
  }

int Automaton::getWho() {
    return state().id();
  }

Automaton::~Automaton() = default;
