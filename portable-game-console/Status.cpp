#include "Status.h"
#include "EvolutionCatalog.h"
#include "CharacterCatalog.h"
#include "SkillCatalog.h"

#include <Arduino.h>
#include <SdFat.h>
#include "Item.h"
#include "ItemCatalog.h"


int Status::getPreEvolution(int currentId) {
    const EvolutionCatalog::Lineage lineage = EvolutionCatalog::getLineage(currentId);
    return lineage.count > 1 ? lineage.ids[1] : -1;
  }

int Status::getHp(){return HP;}

void Status::setHp(int val){ HP = val;}

int Status::getMaxHp(){return MaxHP;}

int Status::getTp(){return TP;}

int Status::getMaxTp(){return MaxTP;}

int Status::getAttack() {
    float finalValue = (float)this->Attack; // 基礎ATK
    // 武器の補正
    if (equipWeaponId != -1) {
      Item weapon = ItemCatalog::get(equipWeaponId);
      finalValue += weapon.value; // 武器の攻撃力を足す
    }
    // アクセの補正 (例: パワーリング ID 500)
    if (equipAccId == 500) finalValue += 10;
    if (equipAccId == 599) finalValue += 500;
    // バフを適用 (例: 1.5倍 or 2.0倍)
    if (atkBuffTurns > 0) {
      finalValue *= atkBuffRate;
    }
    // デバフを適用 (例: 0.7倍 or 0.5倍)
    if (atkDebuffTurns > 0) {
      finalValue *= atkDebuffRate;
    }
    
    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getDefense() {
    float finalValue = (float)this->Defense; // 基礎DEF
    // 防具の補正
    if (equipArmorId != -1) {
      Item armor = ItemCatalog::get(equipArmorId);
      finalValue += armor.value;
    }
    if (equipAccId == 501) finalValue += 10;  // ガードリング
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (defBuffTurns > 0) {
        finalValue *= defBuffRate;
    }
    if (defDebuffTurns > 0) {
        finalValue *= defDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getSpeed() {
    float finalValue = (float)this->Speed; // 基礎SPD
    // アクセ補正
    if (equipAccId == 502) finalValue += 10;  // クイックブーツ
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (spdBuffTurns > 0) {
        finalValue *= spdBuffRate;
    }
    if (spdDebuffTurns > 0) {
        finalValue *= spdDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getSense() {
    float finalValue = (float)this->Sense; // 基礎SEN
    // アクセ補正
    if (equipAccId == 503) finalValue += 10;  // スナイパーレンズ
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア
    if (senBuffTurns > 0) {
        finalValue *= senBuffRate;
    }
    if (senDebuffTurns > 0) {
        finalValue *= senDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getLuck() {
    float finalValue = (float)this->Luck; // 基礎LCK
    // アクセ補正
    if (equipAccId == 504) finalValue += 50;  // ラッキーコイン
    if (equipAccId == 599) finalValue += 500; // トリニティ・コア

    if (lckBuffTurns > 0) {
        finalValue *= lckBuffRate;
    }
    if (lckDebuffTurns > 0) {
        finalValue *= lckDebuffRate;
    }

    if (finalValue < 1) finalValue = 1;
    return (int)finalValue;
  }

int Status::getLevel(){return Level;}

int Status::getXP(){ return currentXP; }

int Status::getNextXP(){ return nextLevelXP; }

int Status::getWho(){ return who; }

const char* Status::getname(){return name;}

int Status::getPoisonTurns() const { return poisonTurns; }

int Status::getParalyzedTurns() const { return paralyzedTurns; }

int Status::getSilencedTurns() const { return silencedTurns; }

int Status::getConfusedTurns() const { return confusedTurns; }

int Status::normalizeWho(int w) {
    if (w >= 0 && w <= 37) return w + 39;
    return w; 
  }

void Status::updateName() {
    name = CharacterCatalog::nameFor(who);
  }

float Status::getEvasionBonus() {
    float bonus = 1.0; // 通常 1.0倍

    if (evaBuffTurns > 0) {
        bonus *= evaBuffRate;
    }
    
    return bonus;
  }

std::vector<Skill>& Status::getLearnedSkills() {
    return learnedSkills;
  }

void Status::forceLearnSkill(int skillId) {
    learnSkill(SkillCatalog::get(skillId));
  }

Status::Status(int level, int w){
    Level = level;
    who = w;
    const CoreStatsData initialStats =
        CharacterCatalog::initialStatsFor(w, level);
    MaxHP = initialStats.maxHp;
    originalMaxHP = MaxHP;
    HP = MaxHP;
    //tune point
    MaxTP = initialStats.maxTp;
    TP = MaxTP;
    //物理攻撃に関係
    Attack = initialStats.attack;
    //防御に関係
    Defense = initialStats.defense;
    //回避率、命中率に関係
    Speed = initialStats.speed;
    //旋律、ノイズ攻撃に使用
    Sense = initialStats.sense;
    // ドロ率、クリ率に関係
    Luck = initialStats.luck;
    currentXP = 0;
    nextLevelXP = calculateNextXP(Level);
    learnedSkills.reserve(10);
    updateName();
    equipWeaponId = -1;
    equipArmorId = -1;
    equipAccId = -1;
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
    // 初期技をテーブルから習得
    checkLevelUpSkills(0, Level); // 0レベルから現在のレベルまで
  }

void Status::resetBattleStats() {
    MaxHP = originalMaxHP; // HP上限を戻す
    if (HP > MaxHP) HP = MaxHP;
    
    // バフ・デバフ・状態異常の全解除
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
    clearPopups();
  }

void Status::boostMaxHP() {
    if (MaxHP == originalMaxHP) { // 重ね掛け防止
      MaxHP = (int)(MaxHP * 1.5);
      HP = (int)(HP * 1.5); // 現在HPも割合で増やす
    }
  }

int Status::takedamage(
      int damage,
      Skill::Category damageCategory
  ){
    // 1. ターン制無敵 (ID 63)
    if (invalidDamageTurns > 0) {
        return 0; // ダメージ0で終了 (回数は減らない)
    }
    // 2. 回数制バリア (ID 55, 64)
    if (damageGuardCount > 0) {
        damageGuardCount--; // ★ 1回消費
        return 0; // ダメージ0で終了
    }
    // 2. アダマン・ボディ
    // 物理ダメージだけを1にする
    else if (physDamageOneTurns > 0 &&
            damageCategory == Skill::Category::PHYSICAL)
    {
        if (damage > 1) {
            damage = 1;
        }
    }
    
    // 3. ラストスタンド (HP1で耐える)
    if (endureTurns > 0 && damage >= HP) {
        damage = HP - 1;
        endureTurns--; 
    }

    HP -= damage;
    if(HP < 0) HP = 0;
    
    return damage;
  }

void Status::ReceiveHeal(int amount) {
    if (HP <= 0) return; // 戦闘不能者は回復しない (蘇生は Revive を使う)
    
    HP += amount;
    if (HP > MaxHP) {
        HP = MaxHP;
    }
  }

void Status::Revive(int hpPercent) {
    if (HP > 0) return; // 既に生きている

    if (hpPercent <= 0) hpPercent = 1;
    if (hpPercent > 100) hpPercent = 100;
    
    // 最大HPの N% で復活
    HP = MaxHP * hpPercent / 100;
  }

void Status::CureStatus(bool cureDebuffs) {
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

void Status::ReceiveTP(int amount) {
    TP += amount;
    if (TP > MaxTP) {
        TP = MaxTP;
    }
    if (TP < 0) {
        TP = 0;
    }
  }

void Status::AddBuff(int skillId) {
    const int DEFAULT_BUFF_TURNS = 3;
    const int LONG_BUFF_TURNS = 5;

    switch (skillId) {
        // --- 主人公 ---
        case 7: // チューニング・アタック (単体 ATK)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 8: // チューニング・ガード (単体 DEF)
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 9: // アレグロ (全体 SPD)
            spdBuffRate = 1.3; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        // --- ストライカー系 ---
        case 21: // パワーチャージ (ATK 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 25: // ソードダンス (ATK/SPD 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 27: // かばう（次の自分の行動終了まで有効）
            coverTurns = 2;
            break;
        case 29: // トリックステップ (SPD/LCK 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            lckBuffRate = 1.5; lckBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 31: // 王者の波動 (味方全体 ATK/DEF 1.5倍)
            atkBuffRate = 1.5; atkBuffTurns = DEFAULT_BUFF_TURNS;
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 32: // キングス・シールド (1ターン物理・旋律反射)
          // 使用した行動の終了時に1減るため、2から開始
          reflectAllTurns = 2;
          break;
        case 34: // バーサーク (ATK 2倍 / DEF 0.5倍)
            atkBuffRate = 2.0; atkBuffTurns = LONG_BUFF_TURNS;
            defDebuffRate = 0.5; defDebuffTurns = LONG_BUFF_TURNS; // (※デバフも付く)
            break;
        case 39: // ミラーズエッジ (回避率 2倍)
            evaBuffRate = 2.0; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 43: // ダーククローク (姿を消す)
            untargetableTurns = DEFAULT_BUFF_TURNS;
            break;

        // --- ガーディアン系 ---
        case 46: // プロテクト (味方単体 DEF 1.5倍)
            defBuffRate = 1.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 48: // ウォール (味方全体 DEF 1.2倍)
            defBuffRate = 1.2; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 49: // 挑発
            tauntTurns = DEFAULT_BUFF_TURNS;
            break;
        case 50: // リフレクト (物理攻撃反射 1回)
            reflectPhysicalTurns = 1;
            break;
        case 51: // ガーディアン魂 (DEF 2倍、SPD 0.5倍)
            defBuffRate = 2.0; defBuffTurns = LONG_BUFF_TURNS;
            spdDebuffRate = 0.5; spdDebuffTurns = LONG_BUFF_TURNS; // (※デバフも付く)
            break;
        case 52: // 鉄壁 (DEF 2.5倍)
            defBuffRate = 2.5; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 53: // カウンター
            counterTurns = DEFAULT_BUFF_TURNS;
            break;
        case 55: // フォースフィールド (ダメージ無効 1回)
            damageGuardCount = 1;
            break;
        case 56: // アダマン・ボディ (物理ダメージ 1)
            physDamageOneTurns = DEFAULT_BUFF_TURNS;
            break;
        case 57: // 自己修復 (HPリジェネ)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 58: // ラストスタンド (HP1で耐える 1回)
            endureTurns = 1;
            break;
        case 59: // ギガンティック・ウォール (味方全体 DEF 2倍)
            defBuffRate = 2.0; defBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 61: // オートリバイブ (自動蘇生 1回)
            autoReviveTurns = 99;
            break;
        case 62: // 最大HP上昇
            boostMaxHP();
            break;
        case 63: // 無敵 (1ターン ダメージ 0)
            // 使用直後のUpdateTurnで1減るため2から開始する
            invalidDamageTurns = 2;
            break;
        case 64: // 全体バリア (ダメージ無効 1回)
            damageGuardCount = 1;
            break;
        case 65: // 旋律反射
            reflectMagicTurns = DEFAULT_BUFF_TURNS;
            break;
        case 67: // ホーリーウォール (属性耐性UP)
            attributeResistTurns = DEFAULT_BUFF_TURNS;
            // (特殊: 属性耐性フラグ)
            break;

        // --- リジェネレーター系 ---
        case 70: // TPリジェネ
            tpRegenTurns = LONG_BUFF_TURNS;
            break;

        // --- クリッパー系 ---
        case 73: // クイックステップ (自身 SPD 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 75: // エアリアルダンス (自身 AGI 1.5倍)
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 77: // エアステップ (味方単体 SPD/AGI 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 79: // ラピッドムーブ (味方全体 SPD 1.5倍)
            spdBuffRate = 1.5; spdBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 81: // ウインドウォール (味方全体 回避率UP 1.5倍)
            evaBuffRate = 1.5; evaBuffTurns = DEFAULT_BUFF_TURNS;
            break;
        case 83: // 精霊の舞 (自身 TPリジェネ)
            tpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 85: // オーバークロック (自身 2回行動)
            twoActionsTurns = 1;
            isDoubleAction = true;
            break;
        case 86: // リミットブレイク (自身 ATK/SPD 最大)
            // 3ターンの間、攻撃と素早さを 4.0倍 (強烈なバフ) にする
            atkBuffRate = 4.0; 
            atkBuffTurns = 3;
            
            spdBuffRate = 4.0; 
            spdBuffTurns = 3;
            break;

        // --- メンダー系 ---
        case 95: // オートリペア (単体 HP自動回復)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 97: // リジェネフィールド (全体 HP自動回復)
            hpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 99: // ホーリー・ウォール (状態異常無効 1回)
            statusImmuneTurns = 1;
            break;
        case 102: // TPリジェネ
            tpRegenTurns = LONG_BUFF_TURNS;
            break;
        case 103: // オラクルフィールド (全体 TP徐々に回復)
            tpRegenTurns = LONG_BUFF_TURNS;
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
    if (statusImmuneTurns > 0) {
      statusImmuneTurns--; // 1回消費して防ぐ
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
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 11: // ノイズ・ジャミング (全体 ATK)
        atkDebuffRate = 0.8; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      // --- ストライカー系 (バフの副作用) ---
      // (case 34 は AddBuff 側で defDebuffRate = 0.5 をセット)
      // --- ギア・ナイト ---
      case 24: // アーマーブレイク (防御 0.7倍)
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- ヴォイド・ストーカー ---
      case 45: // ヴォイド・ショック (高確率で麻痺)
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- ガーディアン系 ---
      case 47: // シールドバッシュ (確率でスタン/麻痺)
        paralyzedTurns = 1;
        break;
        
      // --- ガーディアン系 (バフの副作用) ---
      // (case 51 は AddBuff 側で spdDebuffRate = 0.5 をセット)

      // --- アクセラレーター ---
      case 87: // タービュランス (全体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

      // --- メンダー系 (敵の強化解除) ---
      case 101: // ディスペル (敵単体 強化解除)
        atkBuffTurns = 0; defBuffTurns = 0; spdBuffTurns = 0;
        senBuffTurns = 0; lckBuffTurns = 0; evaBuffTurns = 0;
        hpRegenTurns = 0; tpRegenTurns = 0;
        // (他の特殊バフもすべて解除)
        break;

      // --- ジャマー系 ---
      case 104: // スロウ (敵単体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 105: // ポイズン (毒)
        // 猛毒中なら通常毒で上書きしない
        if (poisonLevel < 2) {
            poisonLevel = 1;
            poisonTurns = DEFAULT_DEBUFF_TURNS;
        }
        break;
      case 106: // パワーダウン (敵単体 ATK 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 107: // ガードダウン (敵単体 DEF 0.7倍)
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 108: // エリア・スロウ (敵全体 SPD 0.7倍)
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 109: // サイレンス (沈黙)
        silencedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 110: // デッドリーポイズン (猛毒)
        poisonLevel = 2;
        poisonTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 112: // エリア・ブレイク (敵全体 ATK/DEF 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
          break;
      case 113: // カオスフィールド (混乱)
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 114: // ディザスター (全状態異常)
        if (poisonLevel < 2) {
            poisonLevel = 1;
            poisonTurns = DEFAULT_DEBUFF_TURNS;
        }
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        silencedTurns = DEFAULT_DEBUFF_TURNS;
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 116: // ロックダウン (行動不能/麻痺)
        paralyzedTurns = DEFAULT_DEBUFF_TURNS;
        break;
      case 117: // バニッシュ (敵単体 強化全解除)
        // (101:ディスペル と同じ効果)
        atkBuffTurns = 0; defBuffTurns = 0; spdBuffTurns = 0;
        senBuffTurns = 0; lckBuffTurns = 0; evaBuffTurns = 0;
        hpRegenTurns = 0; tpRegenTurns = 0;
        break;
      case 118: // ワールド・ダウン (敵全体 全能力 0.7倍)
        atkDebuffRate = 0.7; atkDebuffTurns = DEFAULT_DEBUFF_TURNS;
        defDebuffRate = 0.7; defDebuffTurns = DEFAULT_DEBUFF_TURNS;
        spdDebuffRate = 0.7; spdDebuffTurns = DEFAULT_DEBUFF_TURNS;
        senDebuffRate = 0.7; senDebuffTurns = DEFAULT_DEBUFF_TURNS;
        lckDebuffRate = 0.7; lckDebuffTurns = DEFAULT_DEBUFF_TURNS;
        break;

        // --- はぐれノイズ ---
      case 122: // かく乱 (混乱)
        confusedTurns = DEFAULT_DEBUFF_TURNS;
        break;

        default:
            break;
    }
    return true;
  }

int Status::calculateNextXP(int level) {
    return 10 + (level * level * 5);
  }

void Status::recalculateStats() {
    // 現在のHPが最大HPを超えるのを防ぐため、上昇分だけ加算する
    int oldMaxHP = MaxHP;
    const CoreStatsData stats =
        CharacterCatalog::statsFor(who, Level);
    MaxHP = stats.maxHp;
    originalMaxHP = MaxHP;
    HP += (MaxHP - oldMaxHP); // 上昇分を加算
    if (HP > MaxHP) HP = MaxHP; // (もし全回復させたいなら HP = MaxHP;)

    // TPや他のステータスは再計算
    MaxTP = stats.maxTp;
    TP = MaxTP; // TPは全回復
    Attack = stats.attack;
    Defense = stats.defense;
    Speed = stats.speed;
    Sense = stats.sense;
    Luck = stats.luck;
  }

bool Status::useTP(int cost) {
    if (TP >= cost) {
      TP -= cost;
      return true;
    }
    return false; // TPが足りない
  }

int Status::gainXP(int amount) {
    if (Level >= 99) {
      return 0;
    }

    currentXP += amount;

    bool leveledUp = false;
    bool evolvedDuringGain = false;

    // 一度に複数レベル上がる場合でも、
    // 1レベルずつ処理して、その都度進化判定を行う。
    while (currentXP >= nextLevelXP &&
          Level < 99)
    {
      int previousLevel = Level;

      currentXP -= nextLevelXP;
      Level++;

      nextLevelXP =
          calculateNextXP(Level);

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
              this->Level >= rule.trigger_value)
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
            Level
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
    if (target->isDefending) {
      damage = (int)(damage * 0.7);
    }
    if (target->attributeResistTurns > 0) {
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
    Attack += amount;
    // (将来的に上限なども設定)
  }

void Status::learnSkill(const Skill& newSkill) {
    // (既に覚えていないかチェックしたほうが親切だが、今は省略)
    for (const auto& s : learnedSkills) {
      if (s.id == newSkill.id) {
        return; // すでに覚えているなら何もしない
      }
    }
    // 新しい技なら追加
    learnedSkills.push_back(newSkill);
  }

void Status::evolve(int newWhoId) {
    // 1. 自分のIDを進化先のものに変更
    int oldLevel = Level;
    this->who = newWhoId;
    updateName();
    // 2. 既存のステータス再計算関数を呼び出す
    // (recalculateStats は新しい who ID を使って
    //  decide_MaxHP などを呼び出すので、これでステータスが更新される)
    recalculateStats(); 

    // 3. 進化ボーナスとしてHP/TPを全回復
    this->HP = this->MaxHP;
    this->TP = this->MaxTP;
    checkLevelUpSkills(0, oldLevel);
  }

void Status::checkLevelUpSkills(int oldLevel, int newLevel) {
    
    // チェック対象のID (最初は今の自分)
    int checkID = this->who;

    // ★ 進化の系譜を遡りながら全てチェックするループ
    while (checkID != -1) {
        int normalizedCheckID = normalizeWho(checkID); // 味方IDに統一

        for (int i = 0; i < skillLearningRuleCount; i++) {
          const SkillLearningRule& rule = skillLearningTable[i];
          
          // 「チェック中のID」かつ「レベル条件を満たした」なら
          if (rule.who_id == normalizedCheckID &&
              rule.level <= newLevel &&
              (rule.level > oldLevel || oldLevel == 0)) // 範囲内、または初期化時(0)
          {
            learnSkill(SkillCatalog::get(rule.skill_id)); // 重複チェック付きで習得
          }
        }

        // ★ 次のループのために、進化前のIDを取得する
        checkID = getPreEvolution(checkID);
    }
  }

void Status::UpdateTurn() {
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

int Status::ApplyTurnEffects() {
    int value = 0;

    if (hpRegenTurns > 0) {
      int heal = getMaxHp() * 0.1;
      if (heal < 1) heal = 1;
      ReceiveHeal(heal);
      value = -heal;
    }

    if (tpRegenTurns > 0) {
      ReceiveTP(getMaxTp() * 0.05);
    }

    if (poisonTurns > 0) {
      int poisonRate = (poisonLevel >= 2) ? 20 : 10;
      int poisonDmg = getMaxHp() * poisonRate / 100;

      if (poisonDmg < 1) poisonDmg = 1;

      if (attributeResistTurns > 0) {
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
    file.println(Level);
    file.println(currentXP);
    file.println(HP);
    file.println(TP);
    file.println(equipWeaponId); // ★ 追加
    file.println(equipArmorId);  // ★ 追加
    file.println(equipAccId);    // ★ 追加
    // 覚えているスキルの数とID
    file.println(learnedSkills.size());
    for (const auto& s : learnedSkills) {
      file.println(s.id);
    }
  }

void Status::loadFrom(FsFile& file) {
    // -------------------------
    // 保存値を読み込む
    // -------------------------
    who = file.readStringUntil('\n').toInt();
    Level = file.readStringUntil('\n').toInt();
    currentXP = file.readStringUntil('\n').toInt();

    // HP / TP は再計算で上書きされるので、一旦退避する
    int savedHP = file.readStringUntil('\n').toInt();
    int savedTP = file.readStringUntil('\n').toInt();

    equipWeaponId = file.readStringUntil('\n').toInt();
    equipArmorId = file.readStringUntil('\n').toInt();
    equipAccId = file.readStringUntil('\n').toInt();

    // -------------------------
    // レベル・種類に応じた基礎値を再構築
    // -------------------------
    updateName();

    // ロードしたLevelに合わせて次の必要XPも更新
    nextLevelXP = calculateNextXP(Level);

    recalculateStats();

    // -------------------------
    // 保存時の現在HP / TPを復元
    // -------------------------
    HP = savedHP;
    TP = savedTP;

    // 壊れた/古いセーブデータ対策
    if (HP < 0) HP = 0;
    if (HP > MaxHP) HP = MaxHP;

    if (TP < 0) TP = 0;
    if (TP > MaxTP) TP = MaxTP;

    // -------------------------
    // 習得済みスキル復元
    // -------------------------
    learnedSkills.clear();

    int skillCount = file.readStringUntil('\n').toInt();

    for (int i = 0; i < skillCount; i++) {
      int skillId = file.readStringUntil('\n').toInt();
      learnedSkills.push_back(SkillCatalog::get(skillId));
    }

    // レベル上は覚えているはずなのに、
    // 古いセーブに存在しないスキルがあれば補完
    checkLevelUpSkills(0, Level);
  }

void Status::addPopup(int value, bool isCrit) {
    popupValues.push_back({value, isCrit});
  }

void Status::clearPopups() {
    popupValues.clear();
  }

Status::~Status() {
    learnedSkills.clear();
    std::vector<Skill>().swap(learnedSkills);
    
    popupValues.clear();
    std::vector<PopupData>().swap(popupValues);
  }

const Status::EvolutionRule* const Status::evolutionTable = EvolutionCatalog::getRules().rules;
// サイズも同様に定義
const int Status::evolutionRuleCount = EvolutionCatalog::RULE_COUNT;

const Status::SkillLearningRule Status::skillLearningTable[] = {
  // --- 仲間1号機 (ストライカー系) ---
  { 39, 1, 20 }, // Lv 1: ハードヒット
  { 39, 5, 21 }, // Lv 5: パワーチャージ
  
  { 44, 10, 22 }, // Lv 10: スラッシュ
  { 44, 15, 23 }, // Lv 15: ダブルヒット
  
  { 49, 20, 24 }, // Lv 20: アーマーブレイク
  { 49, 25, 25 }, // Lv 25: ソードダンス
  
  // (3段階目分岐A: ヴァンガード)
  { 54, 35, 26 }, // Lv 30: シールドバッシュ
  { 54, 45, 27 }, // Lv 35: かばう
  
  // (3段階目分岐B: スワッシュバックラー)
  { 55, 35, 28 }, // Lv 30: ファントムエッジ
  { 55, 45, 29 }, // Lv 35: トリックステップ

  // (4段階目分岐A1: オーバーロード)
  { 61, 50, 30 }, // Lv 40: ギガブレイク
  { 61, 60, 31 }, // Lv 42: 王者の波動
  { 61, 70, 32 }, // Lv 45: キングス・シールド
  { 61, 80, 33 }, // Lv 50: デッドエンド

  // (4段階目分岐A2: デストロイヤー)
  { 62, 50, 34 }, // Lv 40: バーサーク
  { 62, 60, 35 }, // Lv 42: カラミティ・エンド
  { 62, 70, 36 }, // Lv 45: トリプルスラッシュ
  { 62, 80, 37 }, // Lv 50: デモリッシュ

  // (4段階目分岐B1: ゼロ・ブレード)
  { 63, 50, 38 }, // Lv 40: ソニックブレイド
  { 63, 60, 39 }, // Lv 42: ミラーズエッジ
  { 63, 70, 40 }, // Lv 45: 刹那
  { 63, 80, 41 }, // Lv 50: 無空

  // (4段階目分岐B2: ヴォイド・ストーカー)
  { 64, 50, 42 }, // Lv 40: アサシネイト
  { 64, 60, 43 }, // Lv 42: ダーククローク
  { 64, 70, 44 }, // Lv 45: ペインサイクル
  { 64, 80, 45 }, // Lv 50: ヴォイド・ショック

  // --- 仲間2号機 (ガーディアン系) ---
  { 40, 1, 46 }, { 40, 5, 47 },   // ガーディアン (who=48)
  { 45, 10, 48 }, { 45, 15, 49 }, // フォートレス (who=49)
  { 50, 20, 50 }, { 50, 25, 51 }, // キャッスル・ウォール (who=50)
  // (分岐A)
  { 56, 35, 52 }, { 56, 45, 53 }, // バスティオン (who=51)
  { 65, 50, 56 }, { 65, 60, 57 }, { 65, 70, 58 }, { 65, 80, 59 }, // アダマンタイト (who=52)
  { 66, 50, 60 }, { 66, 60, 61 }, { 66, 70, 62 }, { 66, 80, 63 }, // インヴィンシブル (who=53)
  // (分岐B)
  { 57, 35, 54 }, { 57, 45, 55 }, // イージス (who=54)
  { 67, 50, 64 }, { 67, 60, 65 }, { 67, 70, 66 }, { 67, 80, 67 }, // ディヴァイン・シールド (who=55)
  { 68, 50, 68 }, { 68, 60, 69 }, { 68, 70, 70 }, { 68, 80, 71 }, // リジェネレーター (who=56)

  // --- 仲間3号機 (クリッパー系) ---
  // (クリッパー who=41)
  { 41, 1, 72 },   // Lv 1: カッティング
  { 41, 5, 73 },   // Lv 5: クイックステップ

  // (ブレード・ダンサー who=46)
  { 46, 10, 74 },  // Lv 10: ツインエッジ
  { 46, 15, 75 },  // Lv 15: エアリアルダンス

  // (サイクロン・ダンサー who=51)
  { 51, 20, 76 },  // Lv 20: スパイラルエッジ
  { 51, 25, 77 },  // Lv 25: エアステップ

  // (テンペスト who=58)
  { 58, 35, 78 },  // Lv 35: ゲイルストライク
  { 58, 45, 79 },  // Lv 45: ラピッドムーブ

  // (4段階目分岐A: シルフィード who=69)
  { 69, 50, 80 },  // Lv 50: インビジブルエッジ
  { 69, 60, 81 },  // Lv 60: ウインドウォール
  { 69, 70, 82 },  // Lv 70: ストームブリンガー
  { 69, 80, 83 },  // Lv 80: 精霊の舞

  // (4段階目分岐B: アクセラレーター who=70)
  { 70, 50, 84 },  // Lv 50: マッハブレイド
  { 70, 60, 85 },  // Lv 60: オーバークロック
  { 70, 70, 86 },  // Lv 70: リミットブレイク
  { 70, 80, 87 },  // Lv 80: タービュランス

  // --- 仲間4号機 (メンダー系) ---
  // (メンダー who=42)
  { 42, 1, 88 },   // Lv 1: リペア
  { 42, 5, 89 },   // Lv 5: キュア

  // (第1進化 who=47)
  { 47, 10, 90 },  // Lv 10: リペア・プラス
  { 47, 15, 91 },  // Lv 15: リフレッシュ

  // (第2進化 who=52)
  { 52, 20, 92 },  // Lv 20: エリア・リペア
  { 52, 25, 93 },  // Lv 25: リザレクション

  // (第3進化 who=59)
  { 59, 35, 94 },  // Lv 35: フル・リペア
  { 59, 45, 95 },  // Lv 45: オートリペア

  // (4段階目分岐A: who=71)
  { 71, 50, 96 },  // Lv 50: エリア・リペア・プラス
  { 71, 60, 97 },  // Lv 60: リジェネフィールド
  { 71, 70, 98 },  // Lv 70: アセンション
  { 71, 80, 99 },  // Lv 80: ホーリー・ウォール

  // (4段階目分岐B: who=72)
  { 72, 50, 100 }, // Lv 50: TPチャージ
  { 72, 60, 101 }, // Lv 60: ディスペル
  { 72, 70, 102 }, // Lv 70: TPリジェネ
  { 72, 80, 103 }, // Lv 80: オラクルフィールド

  // (ジャマー who=43)
  { 43, 1, 104 },  // Lv 1: スロウ
  { 43, 5, 105 },  // Lv 5: ポイズン

  // (第1進化 who=48)
  { 48, 10, 106 }, // Lv 10: パワーダウン
  { 48, 15, 107 }, // Lv 15: ガードダウン

  // (第2進化 who=53)
  { 53, 20, 108 }, // Lv 20: エリア・スロウ
  { 53, 25, 109 }, // Lv 25: サイレンス

  // (第3進化 who=60)
  { 60, 35, 110 }, // Lv 35: デッドリーポイズン
  { 60, 45, 111 }, // Lv 45: TPドレイン

  // (4段階目分岐A: who=73)
  { 73, 50, 112 }, // Lv 50: エリア・ブレイク
  { 73, 60, 113 }, // Lv 60: カオスフィールド
  { 73, 70, 114 }, // Lv 70: ディザスター
  { 73, 80, 115 }, // Lv 80: ペイン・イーター

  // (4段階目分岐B: who=74)
  { 74, 50, 116 }, // Lv 50: ロックダウン
  { 74, 60, 117 }, // Lv 60: バニッシュ
  { 74, 70, 118 }, // Lv 70: ワールド・ダウン
  { 74, 80, 119 }, // Lv 80: エナジー・バーン
  // (はぐれノイズ who=75)
  { 75, 1, 120 },   // Lv 1: ノイズヒット
  { 75, 10, 121 },  // Lv 10: ソニックダガー
  { 75, 20, 122 },  // Lv 20: かく乱
  { 75, 30, 123 },  // Lv 30: ファントムラッシュ

  // (はぐれキング who=76)
  { 76, 40, 124 },   // Lv 1: キングスピア
  { 76, 50, 125 },  // Lv 15: ノイズストーム
  { 76, 60, 126 },  // Lv 30: ロイヤルチャージ
  { 76, 70, 127 }  // Lv 40: ラッキーセブン
};

const int Status::skillLearningRuleCount = sizeof(Status::skillLearningTable) / sizeof(Status::SkillLearningRule);
