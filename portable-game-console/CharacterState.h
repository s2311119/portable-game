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
  friend class EvolutionSystem;
  CoreStats coreStats;
  Equipment equipment;
  StatusEffects statusEffects;
  Progression progression;
  int who;
  const char* nameValue;
  SkillSet skillSet; // 習得済みの旋律リスト
  int getPreEvolution(int currentId);
public:
  const CoreStats& stats() const;
  const Equipment& equipmentState() const;
  const StatusEffects& effects() const;
  const Progression& progress() const;
  void retireFromBattle();
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
  struct PopupData {
    int value;      // 数値
    bool isCritical; // 会心かどうか
  };
  std::vector<PopupData> popupValues; // int から PopupData に変更
  int attack() const;
  int defense() const;
  int speed() const;
  int sense() const;
  int luck() const;
  int id() const;
  const char* name() const;
  int normalizeWho(int w);
  void updateName();
  using EvolutionTriggerType = EvolutionCatalog::EvolutionTriggerType;
  static constexpr EvolutionTriggerType TRIGGER_LEVEL = EvolutionCatalog::TRIGGER_LEVEL;
  static constexpr EvolutionTriggerType TRIGGER_ITEM = EvolutionCatalog::TRIGGER_ITEM;

  using EvolutionRule = EvolutionCatalog::EvolutionRule;

  static const EvolutionRule* const evolutionTable;
  static const int evolutionRuleCount;
  const SkillSet& skills() const;

  void forceLearnSkill(int skillId);
  CharacterState(int level, int w);

  void clearBattleEffects();
  void applyMaxHpIncrease(int increasedMaxHp, int increasedHp);

  int takeDamage(
      int damage,
      Skill::Category damageCategory = Skill::Category::OTHER
  );

  void heal(int amount);

  void revive(int hpPercent);

  void CureStatus(bool cureDebuffs);

  void restoreTP(int amount);



  void recalculateStats();

  bool spendTP(int cost);

  int gainXP(int amount);

  bool isPlayer();

  void addAttack(int amount);

  void learnSkill(int skillId);
  
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
