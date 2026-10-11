#pragma once
#include "EvolutionCatalog.h"

#include "Skill.h"
#include <vector>

class FsFile;

class Status{
private:
  int HP;
  int originalMaxHP;
  int MaxHP;
  //tune point
  int TP;
  int MaxTP;
  //物理攻撃に関係
  int Attack;
  //防御に関係
  int Defense;
  //回避率、命中率に関係
  int Speed;
  //旋律、ノイズ攻撃に使用
  int Sense;
  // ドロ率、クリ率に関係
  int Luck;
  int Level;
  int who;
  const char* name;
  int currentXP;
  int nextLevelXP;
  std::vector<Skill> learnedSkills; // 習得済みの旋律リスト
  int getPreEvolution(int currentId);
public:
  int equipWeaponId = -1; // 武器
  int equipArmorId = -1;  // 防具
  int equipAccId = -1;    // アクセ
  // --- 状態異常 ---
  int poisonTurns = 0;
  int poisonLevel = 0;   // 0: なし, 1: 毒, 2: 猛毒
  int paralyzedTurns = 0;
  int silencedTurns = 0;
  int confusedTurns = 0;
  // (他にも必要に応じて追加)
  int coverTurns = 0;
  // --- バフ/デバフ管理 ---
  // (効果量を float で、持続ターンを int で持つ)
    
  // バフ (Rateは倍率, 1.5 = 1.5倍)
  float atkBuffRate = 0;
  int atkBuffTurns = 0;
  float defBuffRate = 0;
  int defBuffTurns = 0;
  float spdBuffRate = 0;
  int spdBuffTurns = 0;
  float senBuffRate = 0;
  int senBuffTurns = 0;
  float lckBuffRate = 0;
  int lckBuffTurns = 0;
  float evaBuffRate = 0; // 回避率
  int evaBuffTurns = 0;

  // デバフ (Rateは倍率, 0.7 = 0.7倍)
  float atkDebuffRate = 0;
  int atkDebuffTurns = 0;
  float defDebuffRate = 0;
  int defDebuffTurns = 0;
  float spdDebuffRate = 0;
  int spdDebuffTurns = 0;
  float senDebuffRate = 0;
  int senDebuffTurns = 0;
  float lckDebuffRate = 0;
  int lckDebuffTurns = 0;

  // (HP/TPリジェネなどの特殊効果)
  int hpRegenTurns = 0;
  int tpRegenTurns = 0;
  // (その他、無敵、反射など)
  int invalidDamageTurns = 0; 
  int reflectPhysicalTurns = 0;
  int reflectMagicTurns = 0;
  int tauntTurns = 0;
  int twoActionsTurns = 0;
  int untargetableTurns = 0;
  int physDamageOneTurns = 0;
  int endureTurns = 0;
  int counterTurns = 0;
  int autoReviveTurns = 0;
  int statusImmuneTurns = 0;
  int attributeResistTurns = 0;
  int cantMoveTurns = 0;
  int reflectAllTurns = 0;
  int damageGuardCount = 0;
  bool isDoubleAction = 0; // 2回行動
  bool isDefending = false;
  struct PopupData {
    int value;      // 数値
    bool isCritical; // 会心かどうか
  };
  std::vector<PopupData> popupValues; // int から PopupData に変更
  int getHp();
  void setHp(int val);
  int getMaxHp();
  int getTp();
  int getMaxTp();
  int getAttack();
  int getDefense();
  int getSpeed();
  int getSense();
  int getLuck();
  int getLevel();
  int getXP();
  int getNextXP();
  int getWho();
  const char* getname();
  int getPoisonTurns() const;
  int getParalyzedTurns() const;
  int getSilencedTurns() const;
  int getConfusedTurns() const;
  int normalizeWho(int w);
  void updateName();
  using EvolutionTriggerType = EvolutionCatalog::EvolutionTriggerType;
  static constexpr EvolutionTriggerType TRIGGER_LEVEL = EvolutionCatalog::TRIGGER_LEVEL;
  static constexpr EvolutionTriggerType TRIGGER_ITEM = EvolutionCatalog::TRIGGER_ITEM;
  float getEvasionBonus();

  using EvolutionRule = EvolutionCatalog::EvolutionRule;

  static const EvolutionRule* const evolutionTable;
  static const int evolutionRuleCount;
  std::vector<Skill>& getLearnedSkills();

  void forceLearnSkill(int skillId);
  Status(int level, int w);

  void resetBattleStats();
  // ★ MaxHP増加処理 (ID 62)
  void boostMaxHP();

  int takedamage(
      int damage,
      Skill::Category damageCategory = Skill::Category::OTHER
  );

  void ReceiveHeal(int amount);

  void Revive(int hpPercent);

  void CureStatus(bool cureDebuffs);

  void ReceiveTP(int amount);

  void AddBuff(int skillId);
  bool checkDebuffHit(int baseChance, Status* user);
  bool AddDebuff(int skillId, Status* user = nullptr);

  int calculateNextXP(int level);

  void recalculateStats();

  bool useTP(int cost);

  int gainXP(int amount);

  int caclatedamage(int Attack, int Defense, int ASpeed, int DSpeed, int ALuck, int DLuck, bool &retIsCrit);

  int caclulateSkillDamage(Status* target, Skill& skill, bool &retIsCrit);

  int calculateHealAmount(Skill& skill, bool &retIsCrit);

  bool isPlayer();

  void addAttack(int amount);

  void learnSkill(const Skill& newSkill);
  
  void evolve(int newWhoId);

  void checkLevelUpSkills(int oldLevel, int newLevel);
  void UpdateTurn();

  int ApplyTurnEffects();

  void saveTo(FsFile& file);

  void loadFrom(FsFile& file);

  void addPopup(int value, bool isCrit = false);

  void clearPopups();
  ~Status();
};
