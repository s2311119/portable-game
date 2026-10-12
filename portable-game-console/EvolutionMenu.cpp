#include "EvolutionMenu.h"

void EvolutionMenu::open() {
  cursor_ = 0;
}

void EvolutionMenu::moveUp(int memberCount) {
  cursor_ = (cursor_ - 1 + memberCount) % memberCount;
}

void EvolutionMenu::moveDown(int memberCount) {
  cursor_ = (cursor_ + 1) % memberCount;
}

EvolutionRequest EvolutionMenu::confirm() const {
  return {cursor_};
}

EvolutionMenuResult EvolutionMenu::cancel() const {
  return EvolutionMenuResult::BACK;
}

void EvolutionMenu::adjustCursorAfterEvolution(int memberCount) {
  if (cursor_ >= memberCount) {
    cursor_ = memberCount - 1;
  }
}

int EvolutionMenu::cursor() const {
  return cursor_;
}
