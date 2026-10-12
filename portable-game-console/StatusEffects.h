#pragma once

class StatusEffects {
private:
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
public:
  void initialize();
  void clearBattleEffects();
  void cureStatus(bool cureDebuffs);
  void updateTurn();
  float applyAttackModifiers(float finalValue) const;
  float applyDefenseModifiers(float finalValue) const;
  float applySpeedModifiers(float finalValue) const;
  float applySenseModifiers(float finalValue) const;
  float applyLuckModifiers(float finalValue) const;
  float evasionBonus() const;
  int getPoisonTurns() const;
  int getPoisonLevel() const;
  int getParalyzedTurns() const;
  int getSilencedTurns() const;
  int getConfusedTurns() const;
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
  bool getIsDoubleAction() const;
  bool getIsDefending() const;
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
};
