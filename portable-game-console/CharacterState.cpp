#include "CharacterState.h"
#include "CharacterProgressionSystem.h"
#include "CharacterCatalog.h"

#include <SdFat.h>


void CharacterState::equip(EquipmentSlot slot, int itemId) { equipment.equip(slot, itemId); }

void CharacterState::unequip(EquipmentSlot slot) { equipment.unequip(slot); }

int CharacterState::attack() const {
    float finalValue = (float)coreStats.getAttack(); // 基礎ATK
    finalValue = equipment.applyAttackBonus(finalValue);
    // バフを適用 (例: 1.5倍 or 2.0倍)
    finalValue = statusEffects.applyAttackModifiers(finalValue);
    
    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int CharacterState::defense() const {
    float finalValue = (float)coreStats.getDefense(); // 基礎DEF
    finalValue = equipment.applyDefenseBonus(finalValue);
    finalValue = statusEffects.applyDefenseModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int CharacterState::speed() const {
    float finalValue = (float)coreStats.getSpeed(); // 基礎SPD
    finalValue = equipment.applySpeedBonus(finalValue);
    finalValue = statusEffects.applySpeedModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int CharacterState::sense() const {
    float finalValue = (float)coreStats.getSense(); // 基礎SEN
    finalValue = equipment.applySenseBonus(finalValue);
    finalValue = statusEffects.applySenseModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int CharacterState::luck() const {
    float finalValue = (float)coreStats.getLuck(); // 基礎LCK
    finalValue = equipment.applyLuckBonus(finalValue);
    finalValue = statusEffects.applyLuckModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int CharacterState::id() const{ return who; }

const char* CharacterState::name() const{return nameValue;}

void CharacterState::updateName() {
    nameValue = CharacterCatalog::nameFor(who);
  }

const SkillSet& CharacterState::skills() const {
    return skillSet;
  }

void CharacterState::forceLearnSkill(int skillId) {
    learnSkill(skillId);
  }

CharacterState::CharacterState(int level, int w){
    progression.initialize(level);
    who = w;
    const CoreStatsData initialStats =
        CharacterCatalog::initialStatsFor(w, level);
    coreStats.initialize(initialStats);
    skillSet.reserveInitialCapacity();
    updateName();
    unequip(EquipmentSlot::Weapon);
    unequip(EquipmentSlot::Armor);
    unequip(EquipmentSlot::Accessory);
    statusEffects.initialize();
    // 初期技をテーブルから習得
    CharacterProgressionSystem::learnAvailableSkills(*this, 0, progression.level()); // 0レベルから現在のレベルまで
  }

void CharacterState::clearBattleEffects() {
    coreStats.restoreBaseMaxHp();
    
    // バフ・デバフ・状態異常の全解除
    statusEffects.clearBattleEffects();
    clearPopups();
  }

void CharacterState::applyMaxHpIncrease(int increasedMaxHp, int increasedHp) {
    coreStats.applyMaxHpIncrease(increasedMaxHp);
    coreStats.setHp(increasedHp);
  }

int CharacterState::takeDamage(
      int damage,
      Skill::Category damageCategory
  ){
    // 1. ターン制無敵 (ID 63)
    if (statusEffects.getInvalidDamageTurns() > 0) {
        return 0; // ダメージ0で終了 (回数は減らない)
    }
    // 2. 回数制バリア (ID 55, 64)
    if (statusEffects.getDamageGuardCount() > 0) {
        consumeDamageGuard(); // ★ 1回消費
        return 0; // ダメージ0で終了
    }
    // 2. アダマン・ボディ
    // 物理ダメージだけを1にする
    else if (statusEffects.getPhysDamageOneTurns() > 0 &&
            damageCategory == Skill::Category::PHYSICAL)
    {
        if (damage > 1) {
            damage = 1;
        }
    }
    
    // 3. ラストスタンド (HP1で耐える)
    if (statusEffects.getEndureTurns() > 0 && damage >= coreStats.getHp()) {
        damage = coreStats.getHp() - 1;
        consumeLastStand(); 
    }

    coreStats.takeDamage(damage);
    
    return damage;
  }

void CharacterState::heal(int amount) {
    coreStats.heal(amount);
}

void CharacterState::revive(int hpPercent) {
    coreStats.revive(hpPercent);
}

void CharacterState::CureStatus(bool cureDebuffs) {
    statusEffects.cureStatus(cureDebuffs);
  }

void CharacterState::restoreTP(int amount) {
    coreStats.restoreTP(amount);
}

bool CharacterState::spendTP(int cost) {
    return coreStats.spendTP(cost);
}

bool CharacterState::isPlayer() {
    return (who >= 38 && who <= 79); // GDDのID定義 (38以上は味方)
  }

void CharacterState::addAttack(int amount) {
    coreStats.addAttack(amount);
}

void CharacterState::learnSkill(int skillId) {
    skillSet.learn(skillId);
  }

void CharacterState::UpdateTurn() {
    statusEffects.updateTurn();
  }

int CharacterState::ApplyTurnEffects() {
    int value = 0;

    if (statusEffects.getHpRegenTurns() > 0) {
      int heal = stats().getMaxHp() * 0.1;
      if (heal < 1) heal = 1;
      this->heal(heal);
      value = -heal;
    }

    if (statusEffects.getTpRegenTurns() > 0) {
      restoreTP(stats().getMaxTp() * 0.05);
    }

    if (statusEffects.getPoisonTurns() > 0) {
      int poisonRate = (statusEffects.getPoisonLevel() >= 2) ? 20 : 10;
      int poisonDmg = stats().getMaxHp() * poisonRate / 100;

      if (poisonDmg < 1) poisonDmg = 1;

      if (statusEffects.getAttributeResistTurns() > 0) {
        poisonDmg /= 2;
        if (poisonDmg < 1) poisonDmg = 1;
      }

      value =
        takeDamage(poisonDmg);
    }
    return value;
  }

void CharacterState::saveTo(FsFile& file) {
    file.println(who);
    file.println(progression.level());
    file.println(progression.xp());
    file.println(coreStats.getHp());
    file.println(coreStats.getTp());
    file.println(equipmentState().weaponId()); // ★ 追加
    file.println(equipmentState().armorId());  // ★ 追加
    file.println(equipmentState().accessoryId());    // ★ 追加
    // 覚えているスキルの数とID
    file.println(skillSet.count());
    for (int i = 0; i < skillSet.count(); ++i) {
      file.println(skillSet.skillIdAt(i));
    }
  }

void CharacterState::loadFrom(FsFile& file) {
    // -------------------------
    // 保存値を読み込む
    // -------------------------
    who = file.readStringUntil('\n').toInt();
    int savedLevel = file.readStringUntil('\n').toInt();
    int savedXP = file.readStringUntil('\n').toInt();
    progression.restore(savedLevel, savedXP);

    // HP / TP は再計算で上書きされるので、一旦退避する
    int savedHP = file.readStringUntil('\n').toInt();
    int savedTP = file.readStringUntil('\n').toInt();

    equip(EquipmentSlot::Weapon, file.readStringUntil('\n').toInt());
    equip(EquipmentSlot::Armor, file.readStringUntil('\n').toInt());
    equip(EquipmentSlot::Accessory, file.readStringUntil('\n').toInt());

    // -------------------------
    // レベル・種類に応じた基礎値を再構築
    // -------------------------
    updateName();

    // restore() でロードしたLevelに合わせて次の必要XPを更新済み

    CharacterProgressionSystem::recalculateStats(*this);

    // -------------------------
    // 保存時の現在HP / TPを復元
    // -------------------------
    coreStats.restoreCurrentPoints(savedHP, savedTP);

    // -------------------------
    // 習得済みスキル復元
    // -------------------------
    skillSet.clearForRestore();

    int skillCount = file.readStringUntil('\n').toInt();

    for (int i = 0; i < skillCount; i++) {
      int skillId = file.readStringUntil('\n').toInt();
      skillSet.appendRestoredSkillId(skillId);
    }

    // レベル上は覚えているはずなのに、
    // 古いセーブに存在しないスキルがあれば補完
    CharacterProgressionSystem::learnAvailableSkills(*this, 0, progression.level());
  }

void CharacterState::addPopup(int value, bool isCrit) {
    popupValues.push_back({value, isCrit});
  }

void CharacterState::clearPopups() {
    popupValues.clear();
  }

CharacterState::CharacterState(CharacterState&&) noexcept = default;

CharacterState& CharacterState::operator=(CharacterState&&) noexcept = default;

CharacterState::~CharacterState() {
    skillSet.releaseStorage();
    
    popupValues.clear();
    std::vector<PopupData>().swap(popupValues);
  }

void CharacterState::applyAttackBuff(float rate, int turns) { statusEffects.applyAttackBuff(rate, turns); }

void CharacterState::applyDefenseBuff(float rate, int turns) { statusEffects.applyDefenseBuff(rate, turns); }

void CharacterState::applySpeedBuff(float rate, int turns) { statusEffects.applySpeedBuff(rate, turns); }

void CharacterState::applyLuckBuff(float rate, int turns) { statusEffects.applyLuckBuff(rate, turns); }

void CharacterState::applyEvasionBuff(float rate, int turns) { statusEffects.applyEvasionBuff(rate, turns); }

void CharacterState::applyAttackDebuff(float rate, int turns) { statusEffects.applyAttackDebuff(rate, turns); }

void CharacterState::applyDefenseDebuff(float rate, int turns) { statusEffects.applyDefenseDebuff(rate, turns); }

void CharacterState::applySpeedDebuff(float rate, int turns) { statusEffects.applySpeedDebuff(rate, turns); }

void CharacterState::applySenseDebuff(float rate, int turns) { statusEffects.applySenseDebuff(rate, turns); }

void CharacterState::applyLuckDebuff(float rate, int turns) { statusEffects.applyLuckDebuff(rate, turns); }

void CharacterState::applyCover(int turns) { statusEffects.applyCover(turns); }

void CharacterState::applyAllReflection(int turns) { statusEffects.applyAllReflection(turns); }

void CharacterState::applyUntargetable(int turns) { statusEffects.applyUntargetable(turns); }

void CharacterState::applyTaunt(int turns) { statusEffects.applyTaunt(turns); }

void CharacterState::applyPhysicalReflection(int turns) { statusEffects.applyPhysicalReflection(turns); }

void CharacterState::applyCounter(int turns) { statusEffects.applyCounter(turns); }

void CharacterState::applyDamageGuard(int turns) { statusEffects.applyDamageGuard(turns); }

void CharacterState::applyPhysicalDamageOne(int turns) { statusEffects.applyPhysicalDamageOne(turns); }

void CharacterState::applyHpRegeneration(int turns) { statusEffects.applyHpRegeneration(turns); }

void CharacterState::applyLastStand(int turns) { statusEffects.applyLastStand(turns); }

void CharacterState::applyAutoRevive(int turns) { statusEffects.applyAutoRevive(turns); }

void CharacterState::applyInvulnerability(int turns) { statusEffects.applyInvulnerability(turns); }

void CharacterState::applyMagicReflection(int turns) { statusEffects.applyMagicReflection(turns); }

void CharacterState::applyAttributeResistance(int turns) { statusEffects.applyAttributeResistance(turns); }

void CharacterState::applyTpRegeneration(int turns) { statusEffects.applyTpRegeneration(turns); }

void CharacterState::applyStatusImmunity(int turns) { statusEffects.applyStatusImmunity(turns); }

void CharacterState::applyParalysis(int turns) { statusEffects.applyParalysis(turns); }

void CharacterState::applySilence(int turns) { statusEffects.applySilence(turns); }

void CharacterState::applyConfusion(int turns) { statusEffects.applyConfusion(turns); }

void CharacterState::applyPoison(int level, int turns) { statusEffects.applyPoison(level, turns); }

void CharacterState::applyDoubleAction(int turns) { statusEffects.applyDoubleAction(turns); }

void CharacterState::clearStandardBuffs() { statusEffects.clearStandardBuffs(); }

void CharacterState::consumeDamageGuard() { statusEffects.consumeDamageGuard(); }

void CharacterState::consumeLastStand() { statusEffects.consumeLastStand(); }

void CharacterState::consumeStatusImmunity() { statusEffects.consumeStatusImmunity(); }

void CharacterState::consumePhysicalReflection() { statusEffects.consumePhysicalReflection(); }

void CharacterState::consumeAutoRevive() { statusEffects.consumeAutoRevive(); }

void CharacterState::consumeBlockedAction() { statusEffects.consumeBlockedAction(); }

void CharacterState::applyActionBlock(int turns) { statusEffects.applyActionBlock(turns); }

void CharacterState::enableDoubleAction() { statusEffects.enableDoubleAction(); }

void CharacterState::consumeDoubleAction() { statusEffects.consumeDoubleAction(); }

void CharacterState::beginDefense() { statusEffects.beginDefense(); }

void CharacterState::endDefense() { statusEffects.endDefense(); }

const CoreStats& CharacterState::stats() const { return coreStats; }

const Equipment& CharacterState::equipmentState() const { return equipment; }

const StatusEffects& CharacterState::effects() const { return statusEffects; }

const Progression& CharacterState::progress() const { return progression; }

void CharacterState::retireFromBattle() { coreStats.setHp(0); }
