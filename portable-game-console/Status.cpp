#include "Status.h"
#include "SkillLearnsetCatalog.h"
#include "EvolutionCatalog.h"
#include "CharacterCatalog.h"
#include "SkillCatalog.h"

#include <Arduino.h>
#include <SdFat.h>


void Status::equip(EquipmentSlot slot, int itemId) { equipment.equip(slot, itemId); }

void Status::unequip(EquipmentSlot slot) { equipment.unequip(slot); }

int Status::weaponId() const { return equipment.weaponId(); }

int Status::armorId() const { return equipment.armorId(); }

int Status::accessoryId() const { return equipment.accessoryId(); }

int Status::getPreEvolution(int currentId) {
    const EvolutionCatalog::Lineage lineage = EvolutionCatalog::getLineage(currentId);
    return lineage.count > 1 ? lineage.ids[1] : -1;
  }

int Status::getHp(){return coreStats.getHp();}

void Status::setHp(int val){ coreStats.setHp(val);}

int Status::getMaxHp(){return coreStats.getMaxHp();}

int Status::getTp(){return coreStats.getTp();}

int Status::getMaxTp(){return coreStats.getMaxTp();}

int Status::getAttack() {
    float finalValue = (float)coreStats.getAttack(); // 基礎ATK
    finalValue = equipment.applyAttackBonus(finalValue);
    // バフを適用 (例: 1.5倍 or 2.0倍)
    finalValue = statusEffects.applyAttackModifiers(finalValue);
    
    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getDefense() {
    float finalValue = (float)coreStats.getDefense(); // 基礎DEF
    finalValue = equipment.applyDefenseBonus(finalValue);
    finalValue = statusEffects.applyDefenseModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getSpeed() {
    float finalValue = (float)coreStats.getSpeed(); // 基礎SPD
    finalValue = equipment.applySpeedBonus(finalValue);
    finalValue = statusEffects.applySpeedModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getSense() {
    float finalValue = (float)coreStats.getSense(); // 基礎SEN
    finalValue = equipment.applySenseBonus(finalValue);
    finalValue = statusEffects.applySenseModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getLuck() {
    float finalValue = (float)coreStats.getLuck(); // 基礎LCK
    finalValue = equipment.applyLuckBonus(finalValue);
    finalValue = statusEffects.applyLuckModifiers(finalValue);

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getLevel(){return progression.level();}

int Status::getXP(){ return progression.xp(); }

int Status::getNextXP(){ return progression.nextLevelXp(); }

int Status::getWho(){ return who; }

const char* Status::getname(){return name;}

int Status::getPoisonTurns() const { return statusEffects.getPoisonTurns(); }

int Status::getParalyzedTurns() const { return statusEffects.getParalyzedTurns(); }

int Status::getSilencedTurns() const { return statusEffects.getSilencedTurns(); }

int Status::getConfusedTurns() const { return statusEffects.getConfusedTurns(); }

int Status::normalizeWho(int w) {
    if (w >= 0 && w <= 37) return w + 39;
    return w; 
  }

void Status::updateName() {
    name = CharacterCatalog::nameFor(who);
  }

float Status::getEvasionBonus() {
    return statusEffects.evasionBonus();
  }

const SkillSet& Status::skills() const {
    return skillSet;
  }

void Status::forceLearnSkill(int skillId) {
    learnSkill(SkillCatalog::get(skillId));
  }

Status::Status(int level, int w){
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
    checkLevelUpSkills(0, progression.level()); // 0レベルから現在のレベルまで
  }

void Status::resetBattleStats() {
    coreStats.restoreBaseMaxHp();
    
    // バフ・デバフ・状態異常の全解除
    statusEffects.clearBattleEffects();
    clearPopups();
  }

void Status::boostMaxHP() {
    if (coreStats.getMaxHp() == coreStats.getBaseMaxHp()) { // 重ね掛け防止
      coreStats.applyMaxHpIncrease((int)(coreStats.getMaxHp() * 1.5));
      coreStats.setHp((int)(coreStats.getHp() * 1.5)); // 現在HPも割合で増やす
    }
  }

int Status::takedamage(
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

void Status::ReceiveHeal(int amount) {
    coreStats.heal(amount);
}

void Status::Revive(int hpPercent) {
    coreStats.revive(hpPercent);
}

void Status::CureStatus(bool cureDebuffs) {
    statusEffects.cureStatus(cureDebuffs);
  }

void Status::ReceiveTP(int amount) {
    coreStats.restoreTP(amount);
}

void Status::AddBuff(int skillId) {
    const int DEFAULT_BUFF_TURNS = 3;
    const int LONG_BUFF_TURNS = 5;

    switch (skillId) {
        // --- 主人公 ---
        case 7: // チューニング・アタック (単体 ATK)
            applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 8: // チューニング・ガード (単体 DEF)
            applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 9: // アレグロ (全体 SPD)
            applySpeedBuff(1.3, DEFAULT_BUFF_TURNS);
            break;
        // --- ストライカー系 ---
        case 21: // パワーチャージ (ATK 1.5倍)
            applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 25: // ソードダンス (ATK/SPD 1.5倍)
            applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 27: // かばう（次の自分の行動終了まで有効）
            applyCover(2);
            break;
        case 29: // トリックステップ (SPD/LCK 1.5倍)
            applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            applyLuckBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 31: // 王者の波動 (味方全体 ATK/DEF 1.5倍)
            applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 32: // キングス・シールド (1ターン物理・旋律反射)
          // 使用した行動の終了時に1減るため、2から開始
          applyAllReflection(2);
          break;
        case 34: // バーサーク (ATK 2倍 / DEF 0.5倍)
            applyAttackBuff(2.0, LONG_BUFF_TURNS);
            applyDefenseDebuff(0.5, LONG_BUFF_TURNS); // (※デバフも付く)
            break;
        case 39: // ミラーズエッジ (回避率 2倍)
            applyEvasionBuff(2.0, DEFAULT_BUFF_TURNS);
            break;
        case 43: // ダーククローク (姿を消す)
            applyUntargetable(DEFAULT_BUFF_TURNS);
            break;

        // --- ガーディアン系 ---
        case 46: // プロテクト (味方単体 DEF 1.5倍)
            applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 48: // ウォール (味方全体 DEF 1.2倍)
            applyDefenseBuff(1.2, DEFAULT_BUFF_TURNS);
            break;
        case 49: // 挑発
            applyTaunt(DEFAULT_BUFF_TURNS);
            break;
        case 50: // リフレクト (物理攻撃反射 1回)
            applyPhysicalReflection(1);
            break;
        case 51: // ガーディアン魂 (DEF 2倍、SPD 0.5倍)
            applyDefenseBuff(2.0, LONG_BUFF_TURNS);
            applySpeedDebuff(0.5, LONG_BUFF_TURNS); // (※デバフも付く)
            break;
        case 52: // 鉄壁 (DEF 2.5倍)
            applyDefenseBuff(2.5, DEFAULT_BUFF_TURNS);
            break;
        case 53: // カウンター
            applyCounter(DEFAULT_BUFF_TURNS);
            break;
        case 55: // フォースフィールド (ダメージ無効 1回)
            applyDamageGuard(1);
            break;
        case 56: // アダマン・ボディ (物理ダメージ 1)
            applyPhysicalDamageOne(DEFAULT_BUFF_TURNS);
            break;
        case 57: // 自己修復 (HPリジェネ)
            applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 58: // ラストスタンド (HP1で耐える 1回)
            applyLastStand(1);
            break;
        case 59: // ギガンティック・ウォール (味方全体 DEF 2倍)
            applyDefenseBuff(2.0, DEFAULT_BUFF_TURNS);
            break;
        case 61: // オートリバイブ (自動蘇生 1回)
            applyAutoRevive(99);
            break;
        case 62: // 最大HP上昇
            boostMaxHP();
            break;
        case 63: // 無敵 (1ターン ダメージ 0)
            // 使用直後のUpdateTurnで1減るため2から開始する
            applyInvulnerability(2);
            break;
        case 64: // 全体バリア (ダメージ無効 1回)
            applyDamageGuard(1);
            break;
        case 65: // 旋律反射
            applyMagicReflection(DEFAULT_BUFF_TURNS);
            break;
        case 67: // ホーリーウォール (属性耐性UP)
            applyAttributeResistance(DEFAULT_BUFF_TURNS);
            // (特殊: 属性耐性フラグ)
            break;

        // --- リジェネレーター系 ---
        case 70: // TPリジェネ
            applyTpRegeneration(LONG_BUFF_TURNS);
            break;

        // --- クリッパー系 ---
        case 73: // クイックステップ (自身 SPD 1.5倍)
            applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 75: // エアリアルダンス (自身 AGI 1.5倍)
            applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 77: // エアステップ (味方単体 SPD/AGI 1.5倍)
            applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 79: // ラピッドムーブ (味方全体 SPD 1.5倍)
            applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 81: // ウインドウォール (味方全体 回避率UP 1.5倍)
            applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 83: // 精霊の舞 (自身 TPリジェネ)
            applyTpRegeneration(LONG_BUFF_TURNS);
            break;
        case 85: // オーバークロック (自身 2回行動)
            applyDoubleAction(1);
            break;
        case 86: // リミットブレイク (自身 ATK/SPD 最大)
            // 3ターンの間、攻撃と素早さを 4.0倍 (強烈なバフ) にする
            applyAttackBuff(4.0, 3);
            
            applySpeedBuff(4.0, 3);
            break;

        // --- メンダー系 ---
        case 95: // オートリペア (単体 HP自動回復)
            applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 97: // リジェネフィールド (全体 HP自動回復)
            applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 99: // ホーリー・ウォール (状態異常無効 1回)
            applyStatusImmunity(1);
            break;
        case 102: // TPリジェネ
            applyTpRegeneration(LONG_BUFF_TURNS);
            break;
        case 103: // オラクルフィールド (全体 TP徐々に回復)
            applyTpRegeneration(LONG_BUFF_TURNS);
            break;

        default:
            break;
    }
  }

bool Status::checkDebuffHit(int baseChance, Status* user) {
    if (user == nullptr) return true; // ユーザー指定なしなら必中（自分への副作用など）
    
    // 基本確率 + (かけた人の運 - かけられた人の運)
    int chance = baseChance + (user->getLuck() - this->getLuck());
    
    // 最低保証5%、最大95% (完全に無効化したい場合は属性耐性などで処理)
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;

    return (random(0, 100) < chance);
  }

bool Status::AddDebuff(int skillId, Status* user) {
    if (statusEffects.getStatusImmuneTurns() > 0) {
      consumeStatusImmunity(); // 1回消費して防ぐ
      return false; // デバフを適用せずに終了
    }
    const int DEFAULT_DEBUFF_TURNS = 3;
    int baseChance = 100;
    bool sureHit = false;
    // --- 成功率の設定 ---
    switch (skillId) {
      // 毒系
      case 105: baseChance = 80; break; // ポイズン
      case 110: baseChance = 70; break; // デッドリーポイズン
      // 麻痺系
      case 45:  baseChance = 60; break; // ヴォイド・ショック
      case 47:  baseChance = 40; break; // シールドバッシュ(追加効果は低め)
      case 116: baseChance = 50; break; // ロックダウン
      // 精神系
      case 109: baseChance = 70; break; // サイレンス
      case 113: baseChance = 50; break; // カオスフィールド(混乱)
      case 122: baseChance = 50; break; // かく乱
      // ステータスダウン系
      case 10:  baseChance = 90; break; // ラレンタンド
      case 11:  baseChance = 90; break; // ノイズ・ジャミング
      case 24:  baseChance = 80; break; // アーマーブレイク
      case 87:  baseChance = 80; break; // タービュランス
      case 104: baseChance = 85; break; // スロウ
      case 106: baseChance = 85; break; // パワーダウン
      case 107: baseChance = 85; break; // ガードダウン
      case 108: baseChance = 80; break; // エリア・スロウ
      case 112: baseChance = 80; break; // エリア・ブレイク
      case 118: baseChance = 70; break; // ワールド・ダウン
      // 必中系
      case 101: sureHit = true; break; // ディスペル
      case 117: sureHit = true; break; // バニッシュ
      // 特殊
      case 114: baseChance = 40; break; // ディザスター(全体異常)
      default: baseChance = 100; break;
    }
    if (!sureHit && !checkDebuffHit(baseChance, user)) {
      return false;
    }
    switch (skillId) {
      // --- 主人公 ---
      case 10: // ラレンタンド (単体 SPD)
        applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 11: // ノイズ・ジャミング (全体 ATK)
        applyAttackDebuff(0.8, DEFAULT_DEBUFF_TURNS);
        break;
      // --- ストライカー系 (バフの副作用) ---
      // (case 34 は AddBuff 側で defDebuffRate = 0.5 をセット)
      // --- ギア・ナイト ---
      case 24: // アーマーブレイク (防御 0.7倍)
        applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

      // --- ヴォイド・ストーカー ---
      case 45: // ヴォイド・ショック (高確率で麻痺)
        applyParalysis(DEFAULT_DEBUFF_TURNS);
        break;

      // --- ガーディアン系 ---
      case 47: // シールドバッシュ (確率でスタン/麻痺)
        applyParalysis(1);
        break;
        
      // --- ガーディアン系 (バフの副作用) ---
      // (case 51 は AddBuff 側で spdDebuffRate = 0.5 をセット)

      // --- アクセラレーター ---
      case 87: // タービュランス (全体 SPD 0.7倍)
        applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

      // --- メンダー系 (敵の強化解除) ---
      case 101: // ディスペル (敵単体 強化解除)
        clearStandardBuffs();
        // (他の特殊バフもすべて解除)
        break;

      // --- ジャマー系 ---
      case 104: // スロウ (敵単体 SPD 0.7倍)
        applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 105: // ポイズン (毒)
        // 猛毒中なら通常毒で上書きしない
        if (statusEffects.getPoisonLevel() < 2) {
            applyPoison(1, DEFAULT_DEBUFF_TURNS);
        }
        break;
      case 106: // パワーダウン (敵単体 ATK 0.7倍)
        applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 107: // ガードダウン (敵単体 DEF 0.7倍)
        applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 108: // エリア・スロウ (敵全体 SPD 0.7倍)
        applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 109: // サイレンス (沈黙)
        applySilence(DEFAULT_DEBUFF_TURNS);
        break;
      case 110: // デッドリーポイズン (猛毒)
        applyPoison(2, DEFAULT_DEBUFF_TURNS);
        break;
      case 112: // エリア・ブレイク (敵全体 ATK/DEF 0.7倍)
        applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
          break;
      case 113: // カオスフィールド (混乱)
        applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;
      case 114: // ディザスター (全状態異常)
        if (statusEffects.getPoisonLevel() < 2) {
            applyPoison(1, DEFAULT_DEBUFF_TURNS);
        }
        applyParalysis(DEFAULT_DEBUFF_TURNS);
        applySilence(DEFAULT_DEBUFF_TURNS);
        applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;
      case 116: // ロックダウン (行動不能/麻痺)
        applyParalysis(DEFAULT_DEBUFF_TURNS);
        break;
      case 117: // バニッシュ (敵単体 強化全解除)
        // (101:ディスペル と同じ効果)
        clearStandardBuffs();
        break;
      case 118: // ワールド・ダウン (敵全体 全能力 0.7倍)
        applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        applySenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        applyLuckDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

        // --- はぐれノイズ ---
      case 122: // かく乱 (混乱)
        applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;

        default:
            break;
    }
    return true;
  }

int Status::calculateNextXP(int level) {
    return Progression::nextLevelXpFor(level);
  }

void Status::recalculateStats() {
    const CoreStatsData stats =
        CharacterCatalog::statsFor(who, progression.level());
    coreStats.applyRecalculatedStats(stats);
}

bool Status::useTP(int cost) {
    return coreStats.spendTP(cost);
}

int Status::gainXP(int amount) {
    if (progression.level() >= 99) {
      return 0;
    }

    progression.addXP(amount);

    bool leveledUp = false;
    bool evolvedDuringGain = false;

    // 一度に複数レベル上がる場合でも、
    // 1レベルずつ処理して、その都度進化判定を行う。
    while (progression.canLevelUp())
    {
      int previousLevel = progression.level();

      progression.levelUpOnce();

      leveledUp = true;

      // -------------------------------------------------
      // このレベルで発生する自動進化を処理
      // -------------------------------------------------
      bool evolvedAtThisLevel = false;

      while (true) {
        bool evolutionFound = false;

        for (int i = 0;
            i < evolutionRuleCount;
            i++)
        {
          const EvolutionRule& rule =
              evolutionTable[i];

          if (rule.before_who_id == this->who &&
              rule.trigger_type == TRIGGER_LEVEL &&
              progression.level() >= rule.trigger_value)
          {
            evolve(rule.after_who_id);

            evolvedDuringGain = true;
            evolvedAtThisLevel = true;
            evolutionFound = true;

            // who が変わったので、
            // 新しい who でも進化条件があるか再確認する
            break;
          }
        }

        if (!evolutionFound) {
          break;
        }
      }

      // evolve() 内ではステータス再計算と
      // スキル習得処理が行われるので、
      // 進化しなかった場合だけここで処理する。
      if (!evolvedAtThisLevel) {
        recalculateStats();

        checkLevelUpSkills(
            previousLevel,
            progression.level()
        );
      }
    }

    if (evolvedDuringGain) {
      return 2; // レベルアップ中に進化した
    }

    if (leveledUp) {
      return 1; // レベルアップのみ
    }

    return 0; // 変化なし
  }

int Status::caclatedamage(int Attack, int Defense, int ASpeed, int DSpeed, int ALuck, int DLuck, bool &retIsCrit){
    // 回避計算
    int baseHitRate = 95;
    int hitRate = baseHitRate + (ASpeed - DSpeed);
    if(hitRate > 100) hitRate = 100;
    else if(hitRate < 5) hitRate = 5;
    
    bool isHit = (random(0, 100) < hitRate);
    if(!isHit) {
        retIsCrit = false; // ミス
        return 0;
    }

    // クリ計算
    int basecriticalRate = 5;
    int criticalRate = basecriticalRate + (ALuck - DLuck);
    if(criticalRate > 100) criticalRate = 100;
    else if(criticalRate < 0) criticalRate = 0;
    
    // ★ 計算結果を引数の変数に書き込む
    retIsCrit = (random(0, 100) < criticalRate);

    // ダメージ計算
    float effectiveDef = (float)Defense * 0.5;
    if (retIsCrit) effectiveDef *= 0.5; // 貫通

    int damage = (int)((float)Attack - effectiveDef);

    if (retIsCrit) damage = damage * 1.5; // 倍率

    float variance = (float)random(90, 111) / 100.0;
    damage = (int)((float)damage * variance);

    int minDamage = Attack / 20; 
    if (minDamage < 1) minDamage = 1;
    if (damage < minDamage) damage = minDamage;
    damage /= 4;
    return damage;
  }

int Status::caclulateSkillDamage(Status* target, Skill& skill, bool &retIsCrit) {
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
            (this->getSpeed() - target->getSpeed());

        // まず通常状態での「回避率」を求める
        if (hitRate > 100) hitRate = 100;
        if (hitRate < 20) hitRate = 20;

        int missRate = 100 - hitRate;

        // 対象の回避率バフを反映
        // 例：回避率2倍なら、通常5%回避 → 10%回避
        float evasionBonus =
            target->getEvasionBonus();

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
          return 0;
        }
      }
    }
    if ((target->getWho() == 36 || target->getWho() == 37|| target->getWho() == 75 || target->getWho() == 76) && 
        skill.dependence == Skill::StatDependence::SEN) {
      retIsCrit = false;
      return 0; // ダメージ0 (無効)
    }
    // --- 基礎ステータス決定 ---
    int offensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? this->getSense() : this->getAttack();
    int defensiveVal = (skill.dependence == Skill::StatDependence::SEN) ? target->getSense() : target->getDefense();

    // --- 特殊技の威力補正 ---
    float skillPower = (float)skill.power;

    // ID 35: カラミティ・エンド (HPが低いほど高威力)
    if (skill.id == 35) {
        float hpRatio = (float)this->getHp() / (float)this->getMaxHp();
        // HP100%なら1倍、HP1%なら3倍など
        skillPower = skillPower * (3.0 - (hpRatio * 2.0)); 
    }
    // ID 119: エナジー・バーン (敵のTPが多いほど高威力)
    else if (skill.id == 119) {
        skillPower += target->getTp() / 2; 
    }
    // ID 115: ペイン・イーター (状態異常の敵に威力2倍)
    else if (skill.id == 115) {
        if (target->getPoisonTurns() > 0 || target->getParalyzedTurns() > 0 || 
            target->getSilencedTurns() > 0 || target->getConfusedTurns() > 0) {
            skillPower *= 2.0;
        }
    }

    // --- 会心判定 ---
    int critBonus = 0;
    // ID 28, 40: 高クリティカル
    if (skill.id == 28 || skill.id == 40) critBonus = 30; 

    int critRate = 5 + (this->getLuck() - target->getLuck()) + critBonus;
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
    if (target->isDefending()) {
      damage = (int)(damage * 0.7);
    }
    if (target->getAttributeResistTurns() > 0) {
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

    return damage;
  }

int Status::calculateHealAmount(Skill& skill, bool &retIsCrit) {
    int base = this->getSense();
    float multiplier = (float)skill.power / 50.0;
    int heal = (int)((float)base * multiplier);
    
    // --- ★ 会心判定 (回復) ---
    // (回復の会心率は運の10%程度とする)
    int critRate = this->getLuck() / 10; 
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

    return heal;
  }

bool Status::isPlayer() {
    return (who >= 38 && who <= 79); // GDDのID定義 (38以上は味方)
  }

void Status::addAttack(int amount) {
    coreStats.addAttack(amount);
}

void Status::learnSkill(const Skill& newSkill) {
    skillSet.learn(newSkill.id);
  }

void Status::evolve(int newWhoId) {
    // 1. 自分のIDを進化先のものに変更
    int oldLevel = progression.level();
    this->who = newWhoId;
    updateName();
    // 2. 既存のステータス再計算関数を呼び出す
    // (recalculateStats は新しい who ID を使って
    //  decide_MaxHP などを呼び出すので、これでステータスが更新される)
    recalculateStats(); 

    // 3. 進化ボーナスとしてHP/TPを全回復
    coreStats.restoreToFull();
    checkLevelUpSkills(0, oldLevel);
  }

void Status::checkLevelUpSkills(int oldLevel, int newLevel) {
    
    // チェック対象のID (最初は今の自分)
    int checkID = this->who;

    // ★ 進化の系譜を遡りながら全てチェックするループ
    while (checkID != -1) {
        int normalizedCheckID = normalizeWho(checkID); // 味方IDに統一

        const SkillLearnsetCatalog::LearnableSkills skills =
            SkillLearnsetCatalog::getLearnableSkills(
                normalizedCheckID, oldLevel, newLevel);
        for (int i = 0; i < skills.count; i++) {
          learnSkill(SkillCatalog::get(skills.ids[i])); // 重複チェック付きで習得
        }

        // ★ 次のループのために、進化前のIDを取得する
        checkID = getPreEvolution(checkID);
    }
  }

void Status::UpdateTurn() {
    statusEffects.updateTurn();
  }

int Status::ApplyTurnEffects() {
    int value = 0;

    if (statusEffects.getHpRegenTurns() > 0) {
      int heal = getMaxHp() * 0.1;
      if (heal < 1) heal = 1;
      ReceiveHeal(heal);
      value = -heal;
    }

    if (statusEffects.getTpRegenTurns() > 0) {
      ReceiveTP(getMaxTp() * 0.05);
    }

    if (statusEffects.getPoisonTurns() > 0) {
      int poisonRate = (statusEffects.getPoisonLevel() >= 2) ? 20 : 10;
      int poisonDmg = getMaxHp() * poisonRate / 100;

      if (poisonDmg < 1) poisonDmg = 1;

      if (statusEffects.getAttributeResistTurns() > 0) {
        poisonDmg /= 2;
        if (poisonDmg < 1) poisonDmg = 1;
      }

      value =
        takedamage(poisonDmg);
    }
    return value;
  }

void Status::saveTo(FsFile& file) {
    file.println(who);
    file.println(progression.level());
    file.println(progression.xp());
    file.println(coreStats.getHp());
    file.println(coreStats.getTp());
    file.println(weaponId()); // ★ 追加
    file.println(armorId());  // ★ 追加
    file.println(accessoryId());    // ★ 追加
    // 覚えているスキルの数とID
    file.println(skillSet.count());
    for (int i = 0; i < skillSet.count(); ++i) {
      file.println(skillSet.skillIdAt(i));
    }
  }

void Status::loadFrom(FsFile& file) {
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

    recalculateStats();

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
    checkLevelUpSkills(0, progression.level());
  }

void Status::addPopup(int value, bool isCrit) {
    popupValues.push_back({value, isCrit});
  }

void Status::clearPopups() {
    popupValues.clear();
  }

Status::~Status() {
    skillSet.releaseStorage();
    
    popupValues.clear();
    std::vector<PopupData>().swap(popupValues);
  }

const Status::EvolutionRule* const Status::evolutionTable = EvolutionCatalog::getRules().rules;
// サイズも同様に定義
const int Status::evolutionRuleCount = EvolutionCatalog::RULE_COUNT;

int Status::getPoisonLevel() const { return statusEffects.getPoisonLevel(); }

int Status::getCoverTurns() const { return statusEffects.getCoverTurns(); }

float Status::getAtkBuffRate() const { return statusEffects.getAtkBuffRate(); }

int Status::getAtkBuffTurns() const { return statusEffects.getAtkBuffTurns(); }

float Status::getDefBuffRate() const { return statusEffects.getDefBuffRate(); }

int Status::getDefBuffTurns() const { return statusEffects.getDefBuffTurns(); }

float Status::getSpdBuffRate() const { return statusEffects.getSpdBuffRate(); }

int Status::getSpdBuffTurns() const { return statusEffects.getSpdBuffTurns(); }

float Status::getSenBuffRate() const { return statusEffects.getSenBuffRate(); }

int Status::getSenBuffTurns() const { return statusEffects.getSenBuffTurns(); }

float Status::getLckBuffRate() const { return statusEffects.getLckBuffRate(); }

int Status::getLckBuffTurns() const { return statusEffects.getLckBuffTurns(); }

float Status::getEvaBuffRate() const { return statusEffects.getEvaBuffRate(); }

int Status::getEvaBuffTurns() const { return statusEffects.getEvaBuffTurns(); }

float Status::getAtkDebuffRate() const { return statusEffects.getAtkDebuffRate(); }

int Status::getAtkDebuffTurns() const { return statusEffects.getAtkDebuffTurns(); }

float Status::getDefDebuffRate() const { return statusEffects.getDefDebuffRate(); }

int Status::getDefDebuffTurns() const { return statusEffects.getDefDebuffTurns(); }

float Status::getSpdDebuffRate() const { return statusEffects.getSpdDebuffRate(); }

int Status::getSpdDebuffTurns() const { return statusEffects.getSpdDebuffTurns(); }

float Status::getSenDebuffRate() const { return statusEffects.getSenDebuffRate(); }

int Status::getSenDebuffTurns() const { return statusEffects.getSenDebuffTurns(); }

float Status::getLckDebuffRate() const { return statusEffects.getLckDebuffRate(); }

int Status::getLckDebuffTurns() const { return statusEffects.getLckDebuffTurns(); }

int Status::getHpRegenTurns() const { return statusEffects.getHpRegenTurns(); }

int Status::getTpRegenTurns() const { return statusEffects.getTpRegenTurns(); }

int Status::getInvalidDamageTurns() const { return statusEffects.getInvalidDamageTurns(); }

int Status::getReflectPhysicalTurns() const { return statusEffects.getReflectPhysicalTurns(); }

int Status::getReflectMagicTurns() const { return statusEffects.getReflectMagicTurns(); }

int Status::getTauntTurns() const { return statusEffects.getTauntTurns(); }

int Status::getTwoActionsTurns() const { return statusEffects.getTwoActionsTurns(); }

int Status::getUntargetableTurns() const { return statusEffects.getUntargetableTurns(); }

int Status::getPhysDamageOneTurns() const { return statusEffects.getPhysDamageOneTurns(); }

int Status::getEndureTurns() const { return statusEffects.getEndureTurns(); }

int Status::getCounterTurns() const { return statusEffects.getCounterTurns(); }

int Status::getAutoReviveTurns() const { return statusEffects.getAutoReviveTurns(); }

int Status::getStatusImmuneTurns() const { return statusEffects.getStatusImmuneTurns(); }

int Status::getAttributeResistTurns() const { return statusEffects.getAttributeResistTurns(); }

int Status::getCantMoveTurns() const { return statusEffects.getCantMoveTurns(); }

int Status::getReflectAllTurns() const { return statusEffects.getReflectAllTurns(); }

int Status::getDamageGuardCount() const { return statusEffects.getDamageGuardCount(); }

bool Status::hasDoubleAction() const { return statusEffects.getIsDoubleAction(); }

bool Status::isDefending() const { return statusEffects.getIsDefending(); }

void Status::applyAttackBuff(float rate, int turns) { statusEffects.applyAttackBuff(rate, turns); }

void Status::applyDefenseBuff(float rate, int turns) { statusEffects.applyDefenseBuff(rate, turns); }

void Status::applySpeedBuff(float rate, int turns) { statusEffects.applySpeedBuff(rate, turns); }

void Status::applyLuckBuff(float rate, int turns) { statusEffects.applyLuckBuff(rate, turns); }

void Status::applyEvasionBuff(float rate, int turns) { statusEffects.applyEvasionBuff(rate, turns); }

void Status::applyAttackDebuff(float rate, int turns) { statusEffects.applyAttackDebuff(rate, turns); }

void Status::applyDefenseDebuff(float rate, int turns) { statusEffects.applyDefenseDebuff(rate, turns); }

void Status::applySpeedDebuff(float rate, int turns) { statusEffects.applySpeedDebuff(rate, turns); }

void Status::applySenseDebuff(float rate, int turns) { statusEffects.applySenseDebuff(rate, turns); }

void Status::applyLuckDebuff(float rate, int turns) { statusEffects.applyLuckDebuff(rate, turns); }

void Status::applyCover(int turns) { statusEffects.applyCover(turns); }

void Status::applyAllReflection(int turns) { statusEffects.applyAllReflection(turns); }

void Status::applyUntargetable(int turns) { statusEffects.applyUntargetable(turns); }

void Status::applyTaunt(int turns) { statusEffects.applyTaunt(turns); }

void Status::applyPhysicalReflection(int turns) { statusEffects.applyPhysicalReflection(turns); }

void Status::applyCounter(int turns) { statusEffects.applyCounter(turns); }

void Status::applyDamageGuard(int turns) { statusEffects.applyDamageGuard(turns); }

void Status::applyPhysicalDamageOne(int turns) { statusEffects.applyPhysicalDamageOne(turns); }

void Status::applyHpRegeneration(int turns) { statusEffects.applyHpRegeneration(turns); }

void Status::applyLastStand(int turns) { statusEffects.applyLastStand(turns); }

void Status::applyAutoRevive(int turns) { statusEffects.applyAutoRevive(turns); }

void Status::applyInvulnerability(int turns) { statusEffects.applyInvulnerability(turns); }

void Status::applyMagicReflection(int turns) { statusEffects.applyMagicReflection(turns); }

void Status::applyAttributeResistance(int turns) { statusEffects.applyAttributeResistance(turns); }

void Status::applyTpRegeneration(int turns) { statusEffects.applyTpRegeneration(turns); }

void Status::applyStatusImmunity(int turns) { statusEffects.applyStatusImmunity(turns); }

void Status::applyParalysis(int turns) { statusEffects.applyParalysis(turns); }

void Status::applySilence(int turns) { statusEffects.applySilence(turns); }

void Status::applyConfusion(int turns) { statusEffects.applyConfusion(turns); }

void Status::applyPoison(int level, int turns) { statusEffects.applyPoison(level, turns); }

void Status::applyDoubleAction(int turns) { statusEffects.applyDoubleAction(turns); }

void Status::clearStandardBuffs() { statusEffects.clearStandardBuffs(); }

void Status::consumeDamageGuard() { statusEffects.consumeDamageGuard(); }

void Status::consumeLastStand() { statusEffects.consumeLastStand(); }

void Status::consumeStatusImmunity() { statusEffects.consumeStatusImmunity(); }

void Status::consumePhysicalReflection() { statusEffects.consumePhysicalReflection(); }

void Status::consumeAutoRevive() { statusEffects.consumeAutoRevive(); }

void Status::consumeBlockedAction() { statusEffects.consumeBlockedAction(); }

void Status::applyActionBlock(int turns) { statusEffects.applyActionBlock(turns); }

void Status::enableDoubleAction() { statusEffects.enableDoubleAction(); }

void Status::consumeDoubleAction() { statusEffects.consumeDoubleAction(); }

void Status::beginDefense() { statusEffects.beginDefense(); }

void Status::endDefense() { statusEffects.endDefense(); }
