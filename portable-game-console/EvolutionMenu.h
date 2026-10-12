#pragma once

struct EvolutionRequest {
  int partyIndex;
};

enum class EvolutionMenuResult {
  BACK
};

class EvolutionMenu {
private:
  int cursor_ = 0;

public:
  void open();
  void moveUp(int memberCount);
  void moveDown(int memberCount);
  EvolutionRequest confirm() const;
  EvolutionMenuResult cancel() const;
  void adjustCursorAfterEvolution(int memberCount);

  int cursor() const;
};
