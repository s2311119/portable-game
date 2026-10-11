#include "Automaton.h"
#include "CharacterState.h"

Automaton::Automaton(int level, int w) {
    status = new CharacterState(level, w);
  }

const char* Automaton::getname() {
    return status->name();
  }

int Automaton::getWho() {
    return status->id();
  }

Automaton::~Automaton() {
    delete status;
    status = nullptr;
  }
