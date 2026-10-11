#include "CharacterState.h"
#include "SkillLearnsetCatalog.h"
#include "EvolutionCatalog.h"
#include "CharacterCatalog.h"
#include "SkillCatalog.h"

#include <Arduino.h>
#include <SdFat.h>


void CharacterState::equip(EquipmentSlot slot, int itemId) { equipment.equip(slot, itemId); }

void CharacterState::unequip(EquipmentSlot slot) { equipment.unequip(slot); }

int CharacterState::getPreEvolution(int currentId) {
    const EvolutionCatalog::Lineage lineage = EvolutionCatalog::getLineage(currentId);
    return lineage.count > 1 ? lineage.ids[1] : -1;
  }

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

int CharacterState::normalizeWho(int w) {
    if (w >= 0 && w <= 37) return w + 39;
    return w; 
  }

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
    checkLevelUpSkills(0, progression.level()); // 0レベルから現在のレベルまで
  }

void CharacterState::clearBattleEffects() {
    coreStats.restoreBaseMaxHp();
    
    // バフ・デバフ・状態異常の全解除
    statusEffects.clearBattleEffects();
    clearPopups();
  }

void CharacterState::boostMaxHP() {
    if (coreStats.getMaxHp() == coreStats.getBaseMaxHp()) { // 重ね掛け防止
      coreStats.applyMaxHpIncrease((int)(coreStats.getMaxHp() * 1.5));
      coreStats.setHp((int)(coreStats.getHp() * 1.5)); // 現在HPも割合で増やす
    }
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

void CharacterState::AddBuff(int skillId) {
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

bool CharacterState::checkDebuffHit(int baseChance, CharacterState* user) {
    if (user == nullptr) return true; // ユーザー指定なしなら必中（自分への副作用など）
    
    // 基本確率 + (かけた人の運 - かけられた人の運)
    int chance = baseChance + (user->luck() - this->luck());
    
    // 最低保証5%、最大95% (完全に無効化したい場合は属性耐性などで処理)
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;

    return (random(0, 100) < chance);
  }

bool CharacterState::AddDebuff(int skillId, CharacterState* user) {
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

void CharacterState::recalculateStats() {
    const CoreStatsData stats =
        CharacterCatalog::statsFor(who, progression.level());
    coreStats.applyRecalculatedStats(stats);
}

bool CharacterState::spendTP(int cost) {
    return coreStats.spendTP(cost);
}

int CharacterState::gainXP(int amount) {
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

int CharacterState::caclatedamage(int Attack, int Defense, int ASpeed, int DSpeed, int ALuck, int DLuck, bool &retIsCrit){
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

bool CharacterState::isPlayer() {
    return (who >= 38 && who <= 79); // GDDのID定義 (38以上は味方)
  }

void CharacterState::addAttack(int amount) {
    coreStats.addAttack(amount);
}

void CharacterState::learnSkill(int skillId) {
    skillSet.learn(skillId);
  }

void CharacterState::evolve(int newWhoId) {
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

void CharacterState::checkLevelUpSkills(int oldLevel, int newLevel) {
    
    // チェック対象のID (最初は今の自分)
    int checkID = this->who;

    // ★ 進化の系譜を遡りながら全てチェックするループ
    while (checkID != -1) {
        int normalizedCheckID = normalizeWho(checkID); // 味方IDに統一

        const SkillLearnsetCatalog::LearnableSkills skills =
            SkillLearnsetCatalog::getLearnableSkills(
                normalizedCheckID, oldLevel, newLevel);
        for (int i = 0; i < skills.count; i++) {
          learnSkill(skills.ids[i]); // 重複チェック付きで習得
        }

        // ★ 次のループのために、進化前のIDを取得する
        checkID = getPreEvolution(checkID);
    }
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

void CharacterState::addPopup(int value, bool isCrit) {
    popupValues.push_back({value, isCrit});
  }

void CharacterState::clearPopups() {
    popupValues.clear();
  }

CharacterState::~CharacterState() {
    skillSet.releaseStorage();
    
    popupValues.clear();
    std::vector<PopupData>().swap(popupValues);
  }

const CharacterState::EvolutionRule* const CharacterState::evolutionTable = EvolutionCatalog::getRules().rules;
// サイズも同様に定義
const int CharacterState::evolutionRuleCount = EvolutionCatalog::RULE_COUNT;

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
