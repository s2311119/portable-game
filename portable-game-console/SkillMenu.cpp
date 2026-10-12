#include "SkillMenu.h"

#include "Inventory.h"
#include "Player.h"

const int SkillMenu::learnableSkillIds_[] = {
  // --- 攻撃系 ---
  1,  // ノイズ・ブラスト (TP:12)
  2,  // レゾナンス・ウェイブ (TP:25, 全体)
  3,  // ディソナンス (TP:20)

  // --- 回復・治療系 ---
  4,  // ファースト・エイド (TP:8)
  5,  // ピュリファイ (TP:10, 治療)
  6,  // ヒーリング・サークル (TP:30, 全体回復)

  // --- 補助・妨害系 ---
  7,  // チューニング・アタック (TP:15, ATK↑)
  8,  // チューニング・ガード (TP:15, DEF↑)
  9,  // アレグロ (TP:15, 全体SPD↑)
  10, // ラレンタンド (TP:12, 敵SPD↓)
  11, // ノイズ・ジャミング (TP:18, 全体ATK↓)

  // --- 奥義 (コスト高め) ---
  12, // グランド・シンフォニー (TP:60, 全体特大)
  13  // 完全調律 (TP:80, 全体全快)
};

void SkillMenu::open() {
  cursor_ = 0;
}

void SkillMenu::moveUp() {
  cursor_ = (cursor_ - 1 + skillCount()) % skillCount();
}

void SkillMenu::moveDown() {
  cursor_ = (cursor_ + 1) % skillCount();
}

SkillMenuResult SkillMenu::confirm(Inventory& inventory, Player& hero) {
  int skillIdToLearn = skillIdAt(cursor_);
  int requiredScrollId = 200 + skillIdToLearn;

  if (hero.state().skills().has(skillIdToLearn)) {
    return SkillMenuResult::REJECT;
  }
  if (inventory.countItem(requiredScrollId) > 0) {
    inventory.removeItem(requiredScrollId);
    hero.state().learnSkill(skillIdToLearn);
    return SkillMenuResult::LEARNED;
  }
  return SkillMenuResult::REJECT;
}

int SkillMenu::cursor() const {
  return cursor_;
}

int SkillMenu::skillCount() const {
  return sizeof(learnableSkillIds_) / sizeof(learnableSkillIds_[0]);
}

int SkillMenu::skillIdAt(int index) const {
  return learnableSkillIds_[index];
}
