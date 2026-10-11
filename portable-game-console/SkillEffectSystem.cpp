#include "SkillEffectSystem.h"
#include "CharacterState.h"
#include <Arduino.h>

void SkillEffectSystem::applyBuff(int skillId, CharacterState& target) {
    const int DEFAULT_BUFF_TURNS = 3;
    const int LONG_BUFF_TURNS = 5;

    switch (skillId) {
        // --- 主人公 ---
        case 7: // チューニング・アタック (単体 ATK)
            target.applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 8: // チューニング・ガード (単体 DEF)
            target.applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 9: // アレグロ (全体 SPD)
            target.applySpeedBuff(1.3, DEFAULT_BUFF_TURNS);
            break;
        // --- ストライカー系 ---
        case 21: // パワーチャージ (ATK 1.5倍)
            target.applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 25: // ソードダンス (ATK/SPD 1.5倍)
            target.applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            target.applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 27: // かばう（次の自分の行動終了まで有効）
            target.applyCover(2);
            break;
        case 29: // トリックステップ (SPD/LCK 1.5倍)
            target.applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            target.applyLuckBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 31: // 王者の波動 (味方全体 ATK/DEF 1.5倍)
            target.applyAttackBuff(1.5, DEFAULT_BUFF_TURNS);
            target.applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 32: // キングス・シールド (1ターン物理・旋律反射)
          // 使用した行動の終了時に1減るため、2から開始
          target.applyAllReflection(2);
          break;
        case 34: // バーサーク (ATK 2倍 / DEF 0.5倍)
            target.applyAttackBuff(2.0, LONG_BUFF_TURNS);
            target.applyDefenseDebuff(0.5, LONG_BUFF_TURNS); // (※デバフも付く)
            break;
        case 39: // ミラーズエッジ (回避率 2倍)
            target.applyEvasionBuff(2.0, DEFAULT_BUFF_TURNS);
            break;
        case 43: // ダーククローク (姿を消す)
            target.applyUntargetable(DEFAULT_BUFF_TURNS);
            break;

        // --- ガーディアン系 ---
        case 46: // プロテクト (味方単体 DEF 1.5倍)
            target.applyDefenseBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 48: // ウォール (味方全体 DEF 1.2倍)
            target.applyDefenseBuff(1.2, DEFAULT_BUFF_TURNS);
            break;
        case 49: // 挑発
            target.applyTaunt(DEFAULT_BUFF_TURNS);
            break;
        case 50: // リフレクト (物理攻撃反射 1回)
            target.applyPhysicalReflection(1);
            break;
        case 51: // ガーディアン魂 (DEF 2倍、SPD 0.5倍)
            target.applyDefenseBuff(2.0, LONG_BUFF_TURNS);
            target.applySpeedDebuff(0.5, LONG_BUFF_TURNS); // (※デバフも付く)
            break;
        case 52: // 鉄壁 (DEF 2.5倍)
            target.applyDefenseBuff(2.5, DEFAULT_BUFF_TURNS);
            break;
        case 53: // カウンター
            target.applyCounter(DEFAULT_BUFF_TURNS);
            break;
        case 55: // フォースフィールド (ダメージ無効 1回)
            target.applyDamageGuard(1);
            break;
        case 56: // アダマン・ボディ (物理ダメージ 1)
            target.applyPhysicalDamageOne(DEFAULT_BUFF_TURNS);
            break;
        case 57: // 自己修復 (HPリジェネ)
            target.applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 58: // ラストスタンド (HP1で耐える 1回)
            target.applyLastStand(1);
            break;
        case 59: // ギガンティック・ウォール (味方全体 DEF 2倍)
            target.applyDefenseBuff(2.0, DEFAULT_BUFF_TURNS);
            break;
        case 61: // オートリバイブ (自動蘇生 1回)
            target.applyAutoRevive(99);
            break;
        case 62: // 最大HP上昇
            boostMaxHp(target);
            break;
        case 63: // 無敵 (1ターン ダメージ 0)
            // 使用直後のUpdateTurnで1減るため2から開始する
            target.applyInvulnerability(2);
            break;
        case 64: // 全体バリア (ダメージ無効 1回)
            target.applyDamageGuard(1);
            break;
        case 65: // 旋律反射
            target.applyMagicReflection(DEFAULT_BUFF_TURNS);
            break;
        case 67: // ホーリーウォール (属性耐性UP)
            target.applyAttributeResistance(DEFAULT_BUFF_TURNS);
            // (特殊: 属性耐性フラグ)
            break;

        // --- リジェネレーター系 ---
        case 70: // TPリジェネ
            target.applyTpRegeneration(LONG_BUFF_TURNS);
            break;

        // --- クリッパー系 ---
        case 73: // クイックステップ (自身 SPD 1.5倍)
            target.applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 75: // エアリアルダンス (自身 AGI 1.5倍)
            target.applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 77: // エアステップ (味方単体 SPD/AGI 1.5倍)
            target.applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            target.applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 79: // ラピッドムーブ (味方全体 SPD 1.5倍)
            target.applySpeedBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 81: // ウインドウォール (味方全体 回避率UP 1.5倍)
            target.applyEvasionBuff(1.5, DEFAULT_BUFF_TURNS);
            break;
        case 83: // 精霊の舞 (自身 TPリジェネ)
            target.applyTpRegeneration(LONG_BUFF_TURNS);
            break;
        case 85: // オーバークロック (自身 2回行動)
            target.applyDoubleAction(1);
            break;
        case 86: // リミットブレイク (自身 ATK/SPD 最大)
            // 3ターンの間、攻撃と素早さを 4.0倍 (強烈なバフ) にする
            target.applyAttackBuff(4.0, 3);
            
            target.applySpeedBuff(4.0, 3);
            break;

        // --- メンダー系 ---
        case 95: // オートリペア (単体 HP自動回復)
            target.applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 97: // リジェネフィールド (全体 HP自動回復)
            target.applyHpRegeneration(LONG_BUFF_TURNS);
            break;
        case 99: // ホーリー・ウォール (状態異常無効 1回)
            target.applyStatusImmunity(1);
            break;
        case 102: // TPリジェネ
            target.applyTpRegeneration(LONG_BUFF_TURNS);
            break;
        case 103: // オラクルフィールド (全体 TP徐々に回復)
            target.applyTpRegeneration(LONG_BUFF_TURNS);
            break;

        default:
            break;
    }
  }

bool SkillEffectSystem::checkDebuffHit(
    int baseChance,
    const CharacterState& target,
    const CharacterState* user
) {
    if (user == nullptr) return true; // ユーザー指定なしなら必中（自分への副作用など）
    
    // 基本確率 + (かけた人の運 - かけられた人の運)
    int chance = baseChance + (user->luck() - target.luck());
    
    // 最低保証5%、最大95% (完全に無効化したい場合は属性耐性などで処理)
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;

    return (random(0, 100) < chance);
  }

bool SkillEffectSystem::applyDebuff(
    int skillId,
    CharacterState& target,
    const CharacterState* user
) {
    if (target.effects().getStatusImmuneTurns() > 0) {
      target.consumeStatusImmunity(); // 1回消費して防ぐ
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
    if (!sureHit && !checkDebuffHit(baseChance, target, user)) {
      return false;
    }
    switch (skillId) {
      // --- 主人公 ---
      case 10: // ラレンタンド (単体 SPD)
        target.applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 11: // ノイズ・ジャミング (全体 ATK)
        target.applyAttackDebuff(0.8, DEFAULT_DEBUFF_TURNS);
        break;
      // --- ストライカー系 (バフの副作用) ---
      // (case 34 は AddBuff 側で defDebuffRate = 0.5 をセット)
      // --- ギア・ナイト ---
      case 24: // アーマーブレイク (防御 0.7倍)
        target.applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

      // --- ヴォイド・ストーカー ---
      case 45: // ヴォイド・ショック (高確率で麻痺)
        target.applyParalysis(DEFAULT_DEBUFF_TURNS);
        break;

      // --- ガーディアン系 ---
      case 47: // シールドバッシュ (確率でスタン/麻痺)
        target.applyParalysis(1);
        break;
        
      // --- ガーディアン系 (バフの副作用) ---
      // (case 51 は AddBuff 側で spdDebuffRate = 0.5 をセット)

      // --- アクセラレーター ---
      case 87: // タービュランス (全体 SPD 0.7倍)
        target.applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

      // --- メンダー系 (敵の強化解除) ---
      case 101: // ディスペル (敵単体 強化解除)
        target.clearStandardBuffs();
        // (他の特殊バフもすべて解除)
        break;

      // --- ジャマー系 ---
      case 104: // スロウ (敵単体 SPD 0.7倍)
        target.applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 105: // ポイズン (毒)
        // 猛毒中なら通常毒で上書きしない
        if (target.effects().getPoisonLevel() < 2) {
            target.applyPoison(1, DEFAULT_DEBUFF_TURNS);
        }
        break;
      case 106: // パワーダウン (敵単体 ATK 0.7倍)
        target.applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 107: // ガードダウン (敵単体 DEF 0.7倍)
        target.applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 108: // エリア・スロウ (敵全体 SPD 0.7倍)
        target.applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;
      case 109: // サイレンス (沈黙)
        target.applySilence(DEFAULT_DEBUFF_TURNS);
        break;
      case 110: // デッドリーポイズン (猛毒)
        target.applyPoison(2, DEFAULT_DEBUFF_TURNS);
        break;
      case 112: // エリア・ブレイク (敵全体 ATK/DEF 0.7倍)
        target.applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        target.applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
          break;
      case 113: // カオスフィールド (混乱)
        target.applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;
      case 114: // ディザスター (全状態異常)
        if (target.effects().getPoisonLevel() < 2) {
            target.applyPoison(1, DEFAULT_DEBUFF_TURNS);
        }
        target.applyParalysis(DEFAULT_DEBUFF_TURNS);
        target.applySilence(DEFAULT_DEBUFF_TURNS);
        target.applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;
      case 116: // ロックダウン (行動不能/麻痺)
        target.applyParalysis(DEFAULT_DEBUFF_TURNS);
        break;
      case 117: // バニッシュ (敵単体 強化全解除)
        // (101:ディスペル と同じ効果)
        target.clearStandardBuffs();
        break;
      case 118: // ワールド・ダウン (敵全体 全能力 0.7倍)
        target.applyAttackDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        target.applyDefenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        target.applySpeedDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        target.applySenseDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        target.applyLuckDebuff(0.7, DEFAULT_DEBUFF_TURNS);
        break;

        // --- はぐれノイズ ---
      case 122: // かく乱 (混乱)
        target.applyConfusion(DEFAULT_DEBUFF_TURNS);
        break;

        default:
            break;
    }
    return true;
  }

void SkillEffectSystem::boostMaxHp(CharacterState& target) {
    if (target.stats().getMaxHp() == target.stats().getBaseMaxHp()) { // 重ね掛け防止
      const int increasedMaxHp = (int)(target.stats().getMaxHp() * 1.5);
      const int increasedHp = (int)(target.stats().getHp() * 1.5);
      target.applyMaxHpIncrease(increasedMaxHp, increasedHp); // 現在HPも割合で増やす
    }
  }
