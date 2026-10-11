#pragma once

#include <vector>

class Automaton;

class Party {
public:
  // 仲間のポインタを保持するリスト
  std::vector<Automaton*> members;
  std::vector<Automaton*> storage;
  static const int MAX_MEMBERS = 3; // (例: 主人公を除き3体まで)

  Party();

  // ★ 仲間を追加する関数
  int addMember(Automaton* newMember);

  bool swapMember(int memberIdx, int storageIdx);

  // ★★★ 預ける (Deposit) ★★★
  // パーティーから預かり所へ移動 (パーティーは減る)
  bool depositMember(int memberIdx);

  // ★★★ 引き出す (Withdraw) ★★★
  // 預かり所からパーティーへ移動 (パーティーに空きが必要)
  bool withdrawMember(int storageIdx);
  ~Party();
  // (将来的に、仲間を外す関数などもここに追加)
};
