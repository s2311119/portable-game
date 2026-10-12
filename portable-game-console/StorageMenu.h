#pragma once

struct StorageMemberRequest {
  int memberIndex;
};

enum class StorageMenuResult {
  NONE,
  CLOSE
};

class StorageMenu {
public:
  enum class State {
    CHOICE,
    WITHDRAW,
    DEPOSIT
  };

  void open();
  void toggleChoice();
  void moveUp(int count);
  void moveDown(int count);
  void selectChoice();
  StorageMemberRequest requestMember() const;
  void completeWithdraw();
  void completeDeposit(int remainingCount);
  StorageMenuResult cancel();

  State state() const { return state_; }
  int cursor() const { return cursor_; }

private:
  State state_ = State::CHOICE;
  int cursor_ = 0;
};
