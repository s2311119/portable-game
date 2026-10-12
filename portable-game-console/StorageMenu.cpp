#include "StorageMenu.h"

void StorageMenu::open() {
  state_ = State::CHOICE;
  cursor_ = 0;
}

void StorageMenu::toggleChoice() {
  cursor_ = (cursor_ == 0) ? 1 : 0;
}

void StorageMenu::moveUp(int count) {
  cursor_ = (cursor_ - 1 + count) % count;
}

void StorageMenu::moveDown(int count) {
  cursor_ = (cursor_ + 1) % count;
}

void StorageMenu::selectChoice() {
  state_ = (cursor_ == 0) ? State::WITHDRAW : State::DEPOSIT;
  cursor_ = 0;
}

StorageMemberRequest StorageMenu::requestMember() const {
  return {cursor_};
}

void StorageMenu::completeWithdraw() {
  cursor_ = 0;
}

void StorageMenu::completeDeposit(int remainingCount) {
  if (cursor_ >= remainingCount) cursor_ = 0;
}

StorageMenuResult StorageMenu::cancel() {
  if (state_ == State::CHOICE) return StorageMenuResult::CLOSE;
  state_ = State::CHOICE;
  // Returning from the member list leaves its shared cursor unchanged.
  return StorageMenuResult::NONE;
}
