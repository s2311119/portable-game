#include "InnMenu.h"

void InnMenu::open() {
  cursor_ = 0;
}

void InnMenu::moveSelection() {
  cursor_ = (cursor_ == 0) ? 1 : 0;
}

InnMenuResult InnMenu::confirm(int money) const {
  if (cursor_ != 0) return InnMenuResult::CLOSE;
  return money >= cost() ? InnMenuResult::STAY_REQUEST
                         : InnMenuResult::INSUFFICIENT_FUNDS;
}

InnMenuResult InnMenu::cancel() const {
  return InnMenuResult::CLOSE;
}
