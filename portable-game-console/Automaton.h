#pragma once
#include "CharacterState.h"

class Automaton {
public:
  /**
   * @brief 仲間（オートマタ）を生成
   * @param level 初期レベル
   * @param w 仲間の種類ID
   */
  Automaton(int level, int w);

  CharacterState& state();
  const CharacterState& state() const;

  const char* getname();

  int getWho();

  ~Automaton();

private:
  CharacterState stateValue;
};
