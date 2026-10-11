#pragma once

class CharacterState;

class Automaton {
public:
  CharacterState* status;

  /**
   * @brief 仲間（オートマタ）を生成
   * @param level 初期レベル
   * @param w 仲間の種類ID
   */
  Automaton(int level, int w);

  const char* getname();

  int getWho();

  ~Automaton();
};
