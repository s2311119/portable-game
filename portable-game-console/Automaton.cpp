#include "Automaton.h"
#include "CharacterState.h"

Automaton::Automaton(int level, int w) {
    status = new CharacterState(level, w);
  }

const char* Automaton::getname() {
    return status->getname();
  }

int Automaton::getWho() {
    return status->getWho();
  }

Automaton::~Automaton() {
    delete status;
    status = nullptr;
  }
