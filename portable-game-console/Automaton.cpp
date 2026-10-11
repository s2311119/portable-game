#include "Automaton.h"
#include "Status.h"

Automaton::Automaton(int level, int w) {
    status = new Status(level, w);
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
