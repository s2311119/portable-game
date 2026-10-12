#include "StatusEffects.h"

void StatusEffects::initialize() {
    isDoubleAction = false;
    coverTurns = 0;
    physDamageOneTurns = 0;
    endureTurns = 0;
    counterTurns = 0;
    autoReviveTurns = 0;
    statusImmuneTurns = 0;
    attributeResistTurns = 0;
    cantMoveTurns = 0;
    reflectAllTurns = 0;
    damageGuardCount = 0;
    tauntTurns = 0;
    untargetableTurns = 0;
    poisonTurns = 0; paralyzedTurns = 0; silencedTurns = 0; confusedTurns = 0;
    atkBuffRate = 1.0; atkBuffTurns = 0; defBuffRate = 1.0; defBuffTurns = 0;
    spdBuffRate = 1.0; spdBuffTurns = 0; senBuffRate = 1.0; senBuffTurns = 0;
    lckBuffRate = 1.0; lckBuffTurns = 0; evaBuffRate = 1.0; evaBuffTurns = 0;
    atkDebuffRate = 1.0; atkDebuffTurns = 0; defDebuffRate = 1.0; defDebuffTurns = 0;
    spdDebuffRate = 1.0; spdDebuffTurns = 0; senDebuffRate = 1.0; senDebuffTurns = 0;
    lckDebuffRate = 1.0; lckDebuffTurns = 0;
    hpRegenTurns = 0; tpRegenTurns = 0; invalidDamageTurns = 0; 
    reflectPhysicalTurns = 0; reflectMagicTurns = 0;
}

void StatusEffects::clearBattleEffects() {
    coverTurns = 0;
    physDamageOneTurns = 0;
    endureTurns = 0;
    counterTurns = 0;
    autoReviveTurns = 0;
    statusImmuneTurns = 0;
    attributeResistTurns = 0;
    cantMoveTurns = 0;
    reflectAllTurns = 0;
    damageGuardCount = 0;
    tauntTurns = 0;
    untargetableTurns = 0;
    poisonTurns = 0; paralyzedTurns = 0; silencedTurns = 0; confusedTurns = 0;
    atkBuffRate = 1.0; atkBuffTurns = 0; defBuffRate = 1.0; defBuffTurns = 0;
    spdBuffRate = 1.0; spdBuffTurns = 0; senBuffRate = 1.0; senBuffTurns = 0;
    lckBuffRate = 1.0; lckBuffTurns = 0; evaBuffRate = 1.0; evaBuffTurns = 0;
    atkDebuffRate = 1.0; atkDebuffTurns = 0; defDebuffRate = 1.0; defDebuffTurns = 0;
    spdDebuffRate = 1.0; spdDebuffTurns = 0; senDebuffRate = 1.0; senDebuffTurns = 0;
    lckDebuffRate = 1.0; lckDebuffTurns = 0;
    hpRegenTurns = 0; tpRegenTurns = 0; invalidDamageTurns = 0; 
    reflectPhysicalTurns = 0; reflectMagicTurns = 0;
}

void StatusEffects::cureStatus(bool cureDebuffs) {
    // 状態異常フラグをリセット
      poisonTurns = 0;
      poisonLevel = 0;
      paralyzedTurns = 0;
      silencedTurns = 0;
      confusedTurns = 0;

    // (例: スキルID 91 "リフレッシュ" の場合)
    if (cureDebuffs) {
        atkDebuffTurns = 0;
        defDebuffTurns = 0;
        spdDebuffTurns = 0;
    }
  }

void StatusEffects::updateTurn() {
    // -------------------------
    // 通常バフ
    // -------------------------
    if (atkBuffTurns > 0) atkBuffTurns--;
    if (defBuffTurns > 0) defBuffTurns--;
    if (spdBuffTurns > 0) spdBuffTurns--;
    if (senBuffTurns > 0) senBuffTurns--;
    if (lckBuffTurns > 0) lckBuffTurns--;
    if (evaBuffTurns > 0) evaBuffTurns--;

    if (hpRegenTurns > 0) hpRegenTurns--;
    if (tpRegenTurns > 0) tpRegenTurns--;

    // -------------------------
    // 特殊なターン制バフ
    // -------------------------
    if (coverTurns > 0) coverTurns--;
    if (tauntTurns > 0) tauntTurns--;
    if (untargetableTurns > 0) untargetableTurns--;
    if (counterTurns > 0) counterTurns--;
    if (physDamageOneTurns > 0) physDamageOneTurns--;
    if (attributeResistTurns > 0) attributeResistTurns--;

    if (invalidDamageTurns > 0) invalidDamageTurns--;

    // 反射系は現状の仕様を維持。
    // 次のコミットで「ターン制 / 回数制」を整理する。

    // キングス・シールド：ターン制
    if (reflectAllTurns > 0) reflectAllTurns--;

    // 旋律反射：ターン制
    if (reflectMagicTurns > 0) reflectMagicTurns--;

    // -------------------------
    // デバフ
    // -------------------------
    if (atkDebuffTurns > 0) atkDebuffTurns--;
    if (defDebuffTurns > 0) defDebuffTurns--;
    if (spdDebuffTurns > 0) spdDebuffTurns--;
    if (senDebuffTurns > 0) senDebuffTurns--;
    if (lckDebuffTurns > 0) lckDebuffTurns--;

    // -------------------------
    // 状態異常
    // -------------------------
    if (poisonTurns > 0) {
      poisonTurns--;

      if (poisonTurns == 0) {
        poisonLevel = 0;
      }
    }

    if (paralyzedTurns > 0) paralyzedTurns--;
    if (silencedTurns > 0) silencedTurns--;
    if (confusedTurns > 0) confusedTurns--;
  }

float StatusEffects::applyAttackModifiers(float finalValue) const {
    if (atkBuffTurns > 0) {
      finalValue *= atkBuffRate;
    }
    // デバフを適用 (例: 0.7倍 or 0.5倍)
    if (atkDebuffTurns > 0) {
      finalValue *= atkDebuffRate;
    }
    return finalValue;
}

float StatusEffects::applyDefenseModifiers(float finalValue) const {
    if (defBuffTurns > 0) {
        finalValue *= defBuffRate;
    }
    if (defDebuffTurns > 0) {
        finalValue *= defDebuffRate;
    }
    return finalValue;
}

float StatusEffects::applySpeedModifiers(float finalValue) const {
    if (spdBuffTurns > 0) {
        finalValue *= spdBuffRate;
    }
    if (spdDebuffTurns > 0) {
        finalValue *= spdDebuffRate;
    }
    return finalValue;
}

float StatusEffects::applySenseModifiers(float finalValue) const {
    if (senBuffTurns > 0) {
        finalValue *= senBuffRate;
    }
    if (senDebuffTurns > 0) {
        finalValue *= senDebuffRate;
    }
    return finalValue;
}

float StatusEffects::applyLuckModifiers(float finalValue) const {
    if (lckBuffTurns > 0) {
        finalValue *= lckBuffRate;
    }
    if (lckDebuffTurns > 0) {
        finalValue *= lckDebuffRate;
    }
    return finalValue;
}

float StatusEffects::evasionBonus() const {
    float bonus = 1.0; // 通常 1.0倍

    if (evaBuffTurns > 0) {
        bonus *= evaBuffRate;
    }
    
    return bonus;
  }

int StatusEffects::getPoisonTurns() const { return poisonTurns; }

int StatusEffects::getPoisonLevel() const { return poisonLevel; }

int StatusEffects::getParalyzedTurns() const { return paralyzedTurns; }

int StatusEffects::getSilencedTurns() const { return silencedTurns; }

int StatusEffects::getConfusedTurns() const { return confusedTurns; }

int StatusEffects::getCoverTurns() const { return coverTurns; }

float StatusEffects::getAtkBuffRate() const { return atkBuffRate; }

int StatusEffects::getAtkBuffTurns() const { return atkBuffTurns; }

float StatusEffects::getDefBuffRate() const { return defBuffRate; }

int StatusEffects::getDefBuffTurns() const { return defBuffTurns; }

float StatusEffects::getSpdBuffRate() const { return spdBuffRate; }

int StatusEffects::getSpdBuffTurns() const { return spdBuffTurns; }

float StatusEffects::getSenBuffRate() const { return senBuffRate; }

int StatusEffects::getSenBuffTurns() const { return senBuffTurns; }

float StatusEffects::getLckBuffRate() const { return lckBuffRate; }

int StatusEffects::getLckBuffTurns() const { return lckBuffTurns; }

float StatusEffects::getEvaBuffRate() const { return evaBuffRate; }

int StatusEffects::getEvaBuffTurns() const { return evaBuffTurns; }

float StatusEffects::getAtkDebuffRate() const { return atkDebuffRate; }

int StatusEffects::getAtkDebuffTurns() const { return atkDebuffTurns; }

float StatusEffects::getDefDebuffRate() const { return defDebuffRate; }

int StatusEffects::getDefDebuffTurns() const { return defDebuffTurns; }

float StatusEffects::getSpdDebuffRate() const { return spdDebuffRate; }

int StatusEffects::getSpdDebuffTurns() const { return spdDebuffTurns; }

float StatusEffects::getSenDebuffRate() const { return senDebuffRate; }

int StatusEffects::getSenDebuffTurns() const { return senDebuffTurns; }

float StatusEffects::getLckDebuffRate() const { return lckDebuffRate; }

int StatusEffects::getLckDebuffTurns() const { return lckDebuffTurns; }

int StatusEffects::getHpRegenTurns() const { return hpRegenTurns; }

int StatusEffects::getTpRegenTurns() const { return tpRegenTurns; }

int StatusEffects::getInvalidDamageTurns() const { return invalidDamageTurns; }

int StatusEffects::getReflectPhysicalTurns() const { return reflectPhysicalTurns; }

int StatusEffects::getReflectMagicTurns() const { return reflectMagicTurns; }

int StatusEffects::getTauntTurns() const { return tauntTurns; }

int StatusEffects::getTwoActionsTurns() const { return twoActionsTurns; }

int StatusEffects::getUntargetableTurns() const { return untargetableTurns; }

int StatusEffects::getPhysDamageOneTurns() const { return physDamageOneTurns; }

int StatusEffects::getEndureTurns() const { return endureTurns; }

int StatusEffects::getCounterTurns() const { return counterTurns; }

int StatusEffects::getAutoReviveTurns() const { return autoReviveTurns; }

int StatusEffects::getStatusImmuneTurns() const { return statusImmuneTurns; }

int StatusEffects::getAttributeResistTurns() const { return attributeResistTurns; }

int StatusEffects::getCantMoveTurns() const { return cantMoveTurns; }

int StatusEffects::getReflectAllTurns() const { return reflectAllTurns; }

int StatusEffects::getDamageGuardCount() const { return damageGuardCount; }

bool StatusEffects::getIsDoubleAction() const { return isDoubleAction; }

bool StatusEffects::getIsDefending() const { return isDefending; }

void StatusEffects::applyAttackBuff(float rate, int turns) { atkBuffRate = rate; atkBuffTurns = turns; }

void StatusEffects::applyDefenseBuff(float rate, int turns) { defBuffRate = rate; defBuffTurns = turns; }

void StatusEffects::applySpeedBuff(float rate, int turns) { spdBuffRate = rate; spdBuffTurns = turns; }

void StatusEffects::applyLuckBuff(float rate, int turns) { lckBuffRate = rate; lckBuffTurns = turns; }

void StatusEffects::applyEvasionBuff(float rate, int turns) { evaBuffRate = rate; evaBuffTurns = turns; }

void StatusEffects::applyAttackDebuff(float rate, int turns) { atkDebuffRate = rate; atkDebuffTurns = turns; }

void StatusEffects::applyDefenseDebuff(float rate, int turns) { defDebuffRate = rate; defDebuffTurns = turns; }

void StatusEffects::applySpeedDebuff(float rate, int turns) { spdDebuffRate = rate; spdDebuffTurns = turns; }

void StatusEffects::applySenseDebuff(float rate, int turns) { senDebuffRate = rate; senDebuffTurns = turns; }

void StatusEffects::applyLuckDebuff(float rate, int turns) { lckDebuffRate = rate; lckDebuffTurns = turns; }

void StatusEffects::applyCover(int turns) { coverTurns = turns; }

void StatusEffects::applyAllReflection(int turns) { reflectAllTurns = turns; }

void StatusEffects::applyUntargetable(int turns) { untargetableTurns = turns; }

void StatusEffects::applyTaunt(int turns) { tauntTurns = turns; }

void StatusEffects::applyPhysicalReflection(int turns) { reflectPhysicalTurns = turns; }

void StatusEffects::applyCounter(int turns) { counterTurns = turns; }

void StatusEffects::applyDamageGuard(int turns) { damageGuardCount = turns; }

void StatusEffects::applyPhysicalDamageOne(int turns) { physDamageOneTurns = turns; }

void StatusEffects::applyHpRegeneration(int turns) { hpRegenTurns = turns; }

void StatusEffects::applyLastStand(int turns) { endureTurns = turns; }

void StatusEffects::applyAutoRevive(int turns) { autoReviveTurns = turns; }

void StatusEffects::applyInvulnerability(int turns) { invalidDamageTurns = turns; }

void StatusEffects::applyMagicReflection(int turns) { reflectMagicTurns = turns; }

void StatusEffects::applyAttributeResistance(int turns) { attributeResistTurns = turns; }

void StatusEffects::applyTpRegeneration(int turns) { tpRegenTurns = turns; }

void StatusEffects::applyStatusImmunity(int turns) { statusImmuneTurns = turns; }

void StatusEffects::applyParalysis(int turns) { paralyzedTurns = turns; }

void StatusEffects::applySilence(int turns) { silencedTurns = turns; }

void StatusEffects::applyConfusion(int turns) { confusedTurns = turns; }

void StatusEffects::applyPoison(int level, int turns) { poisonLevel = level; poisonTurns = turns; }

void StatusEffects::applyDoubleAction(int turns) { twoActionsTurns = turns; isDoubleAction = true; }

void StatusEffects::clearStandardBuffs() { atkBuffTurns = 0; defBuffTurns = 0; spdBuffTurns = 0; senBuffTurns = 0; lckBuffTurns = 0; evaBuffTurns = 0; hpRegenTurns = 0; tpRegenTurns = 0; }

void StatusEffects::consumeDamageGuard() { damageGuardCount--; }

void StatusEffects::consumeLastStand() { endureTurns--; }

void StatusEffects::consumeStatusImmunity() { statusImmuneTurns--; }

void StatusEffects::consumePhysicalReflection() { reflectPhysicalTurns = 0; }

void StatusEffects::consumeAutoRevive() { autoReviveTurns = 0; }

void StatusEffects::consumeBlockedAction() { cantMoveTurns--; }

void StatusEffects::applyActionBlock(int turns) { cantMoveTurns = turns; }

void StatusEffects::enableDoubleAction() { isDoubleAction = true; }

void StatusEffects::consumeDoubleAction() { isDoubleAction = false; }

void StatusEffects::beginDefense() { isDefending = true; }

void StatusEffects::endDefense() { isDefending = false; }
