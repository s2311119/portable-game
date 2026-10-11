#pragma once

#include "Automaton.h"

#include <cstddef>
#include <vector>

class AutomatonStorage;

class Party {
public:
  static const int MAX_MEMBERS = 3; // (例: 主人公を除き3体まで)

  Party();

  // ★ 仲間を追加する関数
  bool addMember(Automaton&& newMember);
  void appendRestoredMember(Automaton&& restoredMember);
  void clearMembers();
  const Automaton& member(size_t index) const;
  CharacterState& memberState(size_t index);
  size_t memberCount() const;

  bool swapMember(size_t memberIdx, AutomatonStorage& storage, size_t storageIdx);

  // ★★★ 預ける (Deposit) ★★★
  // パーティーから預かり所へ移動 (パーティーは減る)
  bool depositMember(size_t memberIdx, AutomatonStorage& storage);

  // ★★★ 引き出す (Withdraw) ★★★
  // 預かり所からパーティーへ移動 (パーティーに空きが必要)
  ~Party();
  // (将来的に、仲間を外す関数などもここに追加)

private:
  std::vector<Automaton> activeMembers;
};
