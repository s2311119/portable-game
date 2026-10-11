#include "Party.h"
#include "Automaton.h"

Party::Party() {
    members.reserve(MAX_MEMBERS); // メモリ予約
    storage.reserve(20);
  }

int Party::addMember(Automaton* newMember) {
    if (members.size() < MAX_MEMBERS) {
      members.push_back(newMember);
      return 1; // パーティー加入
    } else {
      storage.push_back(newMember);
      return 2; // 預かり所送還
    }
  }

bool Party::swapMember(int memberIdx, int storageIdx) {
    if (memberIdx < 0 || memberIdx >= members.size()) return false;
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    // ポインタを入れ替え
    Automaton* temp = members[memberIdx];
    members[memberIdx] = storage[storageIdx];
    storage[storageIdx] = temp;
    return true;
  }

bool Party::depositMember(int memberIdx) {
    if (memberIdx < 0 || memberIdx >= members.size()) return false;
    // 最後の1人は預けられない制約を入れても良い
    // if (members.size() <= 1) return false; 

    storage.push_back(members[memberIdx]);
    members.erase(members.begin() + memberIdx);
    return true;
  }

bool Party::withdrawMember(int storageIdx) {
    if (members.size() >= MAX_MEMBERS) return false; // 満員
    if (storageIdx < 0 || storageIdx >= storage.size()) return false;

    members.push_back(storage[storageIdx]);
    storage.erase(storage.begin() + storageIdx);
    return true;
  }

Party::~Party() {
    for (auto* m : members) delete m;
    members.clear();
    for (auto* s : storage) delete s;
    storage.clear();
  }
