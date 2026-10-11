#pragma once
#include "CoreStats.h"
#include "Equipment.h"
#include "StatusEffects.h"
#include "Progression.h"
#include "SkillSet.h"
#include "EvolutionCatalog.h"

#include "Skill.h"
#include <vector>

class FsFile;

class CharacterState{
private:
  CoreStats coreStats;
  Equipment equipment;
  StatusEffects statusEffects;
  Progression progression;
  int who;
  const char* name;
  SkillSet skillSet; // 習得済みの旋律リスト
  int getPreEvolution(int currentId);
public:
  int getPoisonLevel() const;
  int getCoverTurns() const;
  float getAtkBuffRate() const;
  int getAtkBuffTurns() const;
  float getDefBuffRate() const;
  int getDefBuffTurns() const;
  float getSpdBuffRate() const;
  int getSpdBuffTurns() const;
  float getSenBuffRate() const;
  int getSenBuffTurns() const;
  float getLckBuffRate() const;
  int getLckBuffTurns() const;
  float getEvaBuffRate() const;
  int getEvaBuffTurns() const;
  float getAtkDebuffRate() const;
  int getAtkDebuffTurns() const;
  float getDefDebuffRate() const;
  int getDefDebuffTurns() const;
  float getSpdDebuffRate() const;
  int getSpdDebuffTurns() const;
  float getSenDebuffRate() const;
  int getSenDebuffTurns() const;
  float getLckDebuffRate() const;
  int getLckDebuffTurns() const;
  int getHpRegenTurns() const;
  int getTpRegenTurns() const;
  int getInvalidDamageTurns() const;
  int getReflectPhysicalTurns() const;
  int getReflectMagicTurns() const;
  int getTauntTurns() const;
  int getTwoActionsTurns() const;
  int getUntargetableTurns() const;
  int getPhysDamageOneTurns() const;
  int getEndureTurns() const;
  int getCounterTurns() const;
  int getAutoReviveTurns() const;
  int getStatusImmuneTurns() const;
  int getAttributeResistTurns() const;
  int getCantMoveTurns() const;
  int getReflectAllTurns() const;
  int getDamageGuardCount() const;
  bool hasDoubleAction() const;
  bool isDefending() const;
  void applyAttackBuff(float rate, int turns);
  void applyDefenseBuff(float rate, int turns);
  void applySpeedBuff(float rate, int turns);
  void applyLuckBuff(float rate, int turns);
  void applyEvasionBuff(float rate, int turns);
  void applyAttackDebuff(float rate, int turns);
  void applyDefenseDebuff(float rate, int turns);
  void applySpeedDebuff(float rate, int turns);
  void applySenseDebuff(float rate, int turns);
  void applyLuckDebuff(float rate, int turns);
  void applyCover(int turns);
  void applyAllReflection(int turns);
  void applyUntargetable(int turns);
  void applyTaunt(int turns);
  void applyPhysicalReflection(int turns);
  void applyCounter(int turns);
  void applyDamageGuard(int turns);
  void applyPhysicalDamageOne(int turns);
  void applyHpRegeneration(int turns);
  void applyLastStand(int turns);
  void applyAutoRevive(int turns);
  void applyInvulnerability(int turns);
  void applyMagicReflection(int turns);
  void applyAttributeResistance(int turns);
  void applyTpRegeneration(int turns);
  void applyStatusImmunity(int turns);
  void applyParalysis(int turns);
  void applySilence(int turns);
  void applyConfusion(int turns);
  void applyPoison(int level, int turns);
  void applyDoubleAction(int turns);
  void clearStandardBuffs();
  void consumeDamageGuard();
  void consumeLastStand();
  void consumeStatusImmunity();
  void consumePhysicalReflection();
  void consumeAutoRevive();
  void consumeBlockedAction();
  void applyActionBlock(int turns);
  void enableDoubleAction();
  void consumeDoubleAction();
  void beginDefense();
  void endDefense();
  void equip(EquipmentSlot slot, int itemId);
  void unequip(EquipmentSlot slot);
  int weaponId() const;
  int armorId() const;
  int accessoryId() const;
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
  const SkillSet& skills() const;

  void forceLearnSkill(int skillId);
  CharacterState(int level, int w);

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
  bool checkDebuffHit(int baseChance, CharacterState* user);
  bool AddDebuff(int skillId, CharacterState* user = nullptr);

  int calculateNextXP(int level);

  void recalculateStats();

  bool useTP(int cost);

  int gainXP(int amount);

  int caclatedamage(int Attack, int Defense, int ASpeed, int DSpeed, int ALuck, int DLuck, bool &retIsCrit);

  int caclulateSkillDamage(CharacterState* target, Skill& skill, bool &retIsCrit);

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
  ~CharacterState();
};
