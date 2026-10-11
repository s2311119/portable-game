#pragma once

#include "Automaton.h"

#include <cstddef>
#include <vector>

class Party {
public:
  // 預かり所は次の独立化CommitでAutomatonStorageへ移す。
  std::vector<Automaton*> storage;
  static const int MAX_MEMBERS = 3; // (例: 主人公を除き3体まで)

  Party();

  // ★ 仲間を追加する関数
  bool addMember(Automaton&& newMember);
  void appendRestoredMember(Automaton&& restoredMember);
  void clearMembers();
  const Automaton& member(size_t index) const;
  CharacterState& memberState(size_t index);
  size_t memberCount() const;

  bool swapMember(int memberIdx, int storageIdx);

  // ★★★ 預ける (Deposit) ★★★
  // パーティーから預かり所へ移動 (パーティーは減る)
  bool depositMember(int memberIdx);

  // ★★★ 引き出す (Withdraw) ★★★
  // 預かり所からパーティーへ移動 (パーティーに空きが必要)
  bool withdrawMember(int storageIdx);
  ~Party();
  // (将来的に、仲間を外す関数などもここに追加)

private:
  std::vector<Automaton> activeMembers;
};
