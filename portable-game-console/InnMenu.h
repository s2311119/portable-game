#pragma once

enum class InnMenuResult {
  STAY_REQUEST,
  INSUFFICIENT_FUNDS,
  CLOSE
};

class InnMenu {
public:
  void open();
  void moveSelection();
  InnMenuResult confirm(int money) const;
  InnMenuResult cancel() const;

  int cursor() const { return cursor_; }
  int cost() const { return 10; }

private:
  int cursor_ = 0;
};
