#include "CombatCalculator.h"
#include "CharacterState.h"
#include "Skill.h"
#include <Arduino.h>

DamageResult CombatCalculator::calculateSkillDamage(
    const CharacterState& attacker,
    const CharacterState& target,
    const Skill& skill
) {
    bool retIsCrit = false;
    // --- 0. 命中判定 (ここが抜けていました) ---
    // 回復・バフ・デバフ・蘇生・治療は必中とする
    // 攻撃系(DAMAGE, MULTI_HIT, SPECIAL)のみ判定を行う
    if (skill.type == Skill::EffectType::DAMAGE || 
      skill.type == Skill::EffectType::MULTI_HIT_DAMAGE ||
      skill.type == Skill::EffectType::SPECIAL) 
    {
      bool isSureHit = false;

      // 回避無視スキル
      if (skill.id == 80) { // インビジブルエッジ
        isSureHit = true;
      }
      // 魔法(旋律)攻撃も基本必中とするならここに追加(今回は物理と同じ扱いで計算)

      if (!isSureHit) {
        // 基本命中率
        int hitRate =
            95 +
            (attacker.speed() - target.speed());

        // まず通常状態での「回避率」を求める
        if (hitRate > 100) hitRate = 100;
        if (hitRate < 20) hitRate = 20;

        int missRate = 100 - hitRate;

        // 対象の回避率バフを反映
        // 例：回避率2倍なら、通常5%回避 → 10%回避
        float evasionBonus =
            target.effects().evasionBonus();

        missRate =
            static_cast<int>(
                missRate * evasionBonus
            );

        hitRate = 100 - missRate;

        // 最終的な命中率も20～100%に制限
        if (hitRate > 100) hitRate = 100;
        if (hitRate < 20) hitRate = 20;

        // 命中判定
        if (random(0, 100) >= hitRate) {
          return {false, retIsCrit, 0};
        }
      }
    }
    if ((target.id() == 36 || target.id() == 37|| target.id() == 75 || target.id() == 76) && 
        skill.dependence == Skill::StatDependence::SEN) {
      retIsCrit = false;
      return {true, retIsCrit, 0}; // ダメージ0 (無効)
    }
    // --- 基礎ステータス決定 ---
    int offensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? attacker.sense() : attacker.attack();
    int defensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? target.sense() : target.defense();

    // --- 特殊技の威力補正 ---
    float skillPower = (float)skill.power;

    // ID 35: カラミティ・エンド (HPが低いほど高威力)
    if (skill.id == 35) {
        float hpRatio = (float)attacker.stats().getHp() / (float)attacker.stats().getMaxHp();
        // HP100%なら1倍、HP1%なら3倍など
        skillPower = skillPower * (3.0 - (hpRatio * 2.0)); 
    }
    // ID 119: エナジー・バーン (敵のTPが多いほど高威力)
    else if (skill.id == 119) {
        skillPower += target.stats().getTp() / 2; 
    }
    // ID 115: ペイン・イーター (状態異常の敵に威力2倍)
    else if (skill.id == 115) {
        if (target.effects().getPoisonTurns() > 0 || target.effects().getParalyzedTurns() > 0 || 
            target.effects().getSilencedTurns() > 0 || target.effects().getConfusedTurns() > 0) {
            skillPower *= 2.0;
        }
    }

    // --- 会心判定 ---
    int critBonus = 0;
    // ID 28, 40: 高クリティカル
    if (skill.id == 28 || skill.id == 40) critBonus = 30; 

    int critRate = 5 + (attacker.luck() - target.luck()) + critBonus;
    if(critRate > 100) critRate = 100;
    if(critRate < 0) critRate = 0;
    retIsCrit = (random(0, 100) < critRate);

    // --- 防御力計算 ---
    float effectiveDef = (float)defensiveVal / 2.0;

    // ID 24, 124: 防御無視 (アーマーブレイク, キングスピア)
    if (skill.id == 24 || skill.id == 124) {
        effectiveDef = 0;
    }
    // 会心時は貫通
    else if (retIsCrit) {
        effectiveDef *= 0.5; 
    }

    // --- 最終ダメージ計算 ---
    // (power 50 を基準倍率 1.0 とする)
    float powerMultiplier = skillPower / 50.0;
    int baseDamage = (int)((float)offensiveVal * powerMultiplier);
    
    int damage = baseDamage - (int)effectiveDef;
    if (target.effects().getIsDefending()) {
      damage = (int)(damage * 0.7);
    }
    if (target.effects().getAttributeResistTurns() > 0) {
      damage = damage / 4;
    }
    // 会心ボーナス (1.5倍)
    if (retIsCrit) damage = damage * 1.5;

    // 乱数
    float variance = (float)random(90, 111) / 100.0;
    damage = (int)((float)damage * variance);
    damage /= 4;
    // 最低保証
    int minDamage = offensiveVal / 20;
    if (minDamage < 1) minDamage = 1;
    if (damage < minDamage) damage = minDamage;

    return {true, retIsCrit, damage};
  }

HealResult CombatCalculator::calculateHealAmount(
    const CharacterState& healer,
    const Skill& skill
) {
    bool retIsCrit = false;
    int base = healer.sense();
    float multiplier = (float)skill.power / 50.0;
    int heal = (int)((float)base * multiplier);
    
    // --- ★ 会心判定 (回復) ---
    // (回復の会心率は運の10%程度とする)
    int critRate = healer.luck() / 10; 
    if (critRate < 5) critRate = 5; // 最低5%
    
    retIsCrit = (random(0, 100) < critRate);

    if (retIsCrit) heal = heal * 2.0; // 会心なら1.5倍回復
    // ------------------------
    
    float variance =
        (float)random(90, 111) / 100.0;

    heal =
        (int)((float)heal * variance);

    // ゲームバランス用の最終補正
    heal /= 4;

    // 最終結果として最低1は回復する
    if (heal < 1) {
      heal = 1;
    }

    return {retIsCrit, heal};
  }
