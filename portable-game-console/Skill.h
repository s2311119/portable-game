#pragma once

class Skill {
public:
  int id;
  const char* name;
  int tpCost; // ★消費TP (ゼンマイ)
  int power;  // ★技の威力 (Senseステータスで計算)
  enum class StatDependence {
    NONE, // 依存なし (固定値など)
    ATK,  // 物理 (Attack vs Defense)
    SEN   // 魔法/技術 (Sense vs Sense)
  };
  // main.ino (Skill クラスのコンストラクタ 322行目あたり)

  enum class EffectType {
    NONE,             // 効果なし
    DAMAGE,           // 物理・属性ダメージ (powerを参照)
    HEAL,             // HP回復 (powerを参照)
    CURE_STATUS,      // 状態異常の治療 (例: キュア)
    BUFF,             // 強化 (例: パワーチャージ, クイックステップ)
    DEBUFF,           // 弱体化 (例: スロウ, パワーダウン)
    REVIVE,           // 蘇生 (例: リザレクション)
    MULTI_HIT_DAMAGE, // 多段ヒット (例: ツインエッジ)
    SPECIAL           // その他 (例: 逃走, 仲間を呼ぶ, 複合効果など)
  };

  // スキルの「対象範囲」
  enum class TargetScope {
    SELF,             // 使用者自身
    SINGLE_ALLY,      // 味方単体
    SINGLE_ENEMY,     // 敵単体
    ALL_ALLIES,       // 味方全体
    ALL_ENEMIES,      // 敵全体
    RANDOM_ENEMY      // 敵ランダム (複数回ヒット用など)
  };
  EffectType type;      // スキルの効果タイプ
  TargetScope scope;    // スキルの対象範囲
  StatDependence dependence;
  enum class Category {
    PHYSICAL, // 物理
    MELODY,   // 旋律 (魔法)
    OTHER     // その他 (バフなど)
  };
  Category category;

  Skill(int recid);
};
