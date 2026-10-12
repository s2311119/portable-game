#include "SkillCatalog.h"

Skill SkillCatalog::get(int recid) {
  Skill skill{};
  using StatDependence = Skill::StatDependence;
  using EffectType = Skill::EffectType;
  using TargetScope = Skill::TargetScope;
  using Category = Skill::Category;
  int& id = skill.id;
  const char*& name = skill.name;
  int& tpCost = skill.tpCost;
  int& power = skill.power;
  EffectType& type = skill.type;
  TargetScope& scope = skill.scope;
  StatDependence& dependence = skill.dependence;
  Category& category = skill.category;
    id = recid;

    // デフォルト値
    name = "？？？";
    tpCost = 0;
    power = 0;

    type = EffectType::DAMAGE;
    scope = TargetScope::SINGLE_ENEMY;
    dependence = StatDependence::ATK;
    category = Category::PHYSICAL;

    switch (id) {
      // --- 主人公 (ID 0-19) ---
      case 0: name = "プチノイズ"; tpCost = 5; power = 10; dependence = StatDependence::SEN; break; // 魔法っぽいのでSEN
      case 1: name = "ノイズ・ブラスト"; tpCost = 3; power = 60; dependence = StatDependence::SEN; break;
      case 2: name = "レゾナンス・ウェイブ"; tpCost = 6; power = 40; scope = TargetScope::ALL_ENEMIES; dependence = StatDependence::SEN; break;
      case 3: name = "ディソナンス"; tpCost = 14; power = 120; dependence = StatDependence::SEN; break;
      // (回復・補助系は dependence はあまり関係ないが、回復量はSEN依存にするのが一般的)
      case 4: name = "ファースト・エイド"; tpCost = 8; power = 40; type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; dependence = StatDependence::SEN; break;
      case 5: name = "ピュリファイ"; tpCost = 10; power = 0; type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; break;
      case 6: name = "ヒーリング・サークル"; tpCost = 20; power = 80; type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; dependence = StatDependence::SEN; break;
      case 7: 
        name = "チューニング・アタック"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // 味方単体ATKアップ
      case 8: 
        name = "チューニング・ガード"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // 味方単体DEFアップ
      case 9: 
        name = "アレグロ"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // 味方全体SPDアップ
      case 10: 
        name = "ラレンタンド"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // 敵単体SPDダウン
      case 11: 
        name = "ノイズ・ジャミング"; tpCost = 10; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // 敵全体ATKダウン
      // (バフ・デバフ系は威力0なので省略、奥義はSEN)
      case 12: name = "グランド・シンフォニー"; tpCost = 60; power = 300; scope = TargetScope::ALL_ENEMIES; dependence = StatDependence::SEN; break;
      case 13: name = "完全調律"; tpCost = 60; power = 999; type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; dependence = StatDependence::SEN; break;
      // --- ストライカー (who=39) ---
      case 20: 
        name = "ハードヒット"; tpCost = 5; power = 80; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 21: 
        name = "パワーチャージ"; tpCost = 8; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK 1.5倍バフなど)

      // --- ギア・ソルジャー (who=40) ---
      case 22: 
        name = "スラッシュ"; tpCost = 8; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 23: 
        name = "ダブルヒット"; tpCost = 10; power = 100; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (30ダメージ x 2回)

      // --- ギア・ナイト (who=41) ---
      case 24: 
        name = "アーマーブレイク"; tpCost = 10; power = 150; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (防御無視 or 防御ダウン)
      case 25: 
        name = "ソードダンス"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK/SPD バフ)

      // --- ヴァンガード (who=42) ---
      case 26: 
        name = "シールドスマッシュ"; tpCost = 20; power = 300; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 27: 
        name = "かばう"; tpCost = 5; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (仲間を守る)

      // --- スワッシュバックラー (who=45) ---
      case 28: 
        name = "ファントムエッジ"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (クリティカル率 高)
      case 29: 
        name = "トリックステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (SPD/LCK バフ)
      
      // --- オーバーロード (who=43) ---
      case 30: 
        name = "ギガブレイク"; tpCost = 40; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 31: 
        name = "王者の波動"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 ATK/DEF バフ)
      case 32: 
        name = "キングス・シールド"; tpCost = 10; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (物理・旋律反射)
      case 33: 
        name = "デッドエンド"; tpCost = 60; power = 600; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;

      // --- デストロイヤー (who=44) ---
      case 34: 
        name = "バーサーク"; tpCost = 3; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (ATK 2倍 / DEF 0.5倍)
      case 35: 
        name = "カラミティ・エンド"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (HPが低いほど高威力)
      case 36: 
        name = "トリプルスラッシュ"; tpCost = 15; power = 150; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (80ダメージ x 3回)
      case 37: 
        name = "デモリッシュ"; tpCost = 55; power = 600;
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;

      // --- ゼロ・ブレード (who=46) ---
      case 38: 
        name = "ソニックブレイド"; tpCost = 10; power = 100; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制)
      case 39: 
        name = "ミラーズエッジ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (回避率 2倍バフ)
      case 40: 
        name = "刹那"; tpCost = 20; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高クリティカル率)
      case 41: 
        name = "無空"; tpCost = 65; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (TP消費が激しい)

      // --- ヴォイド・ストーカー (who=47) ---
      case 42: 
        name = "アサシネイト"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (確率で即死)
      case 43: 
        name = "ダーククローク"; tpCost = 3; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (姿を消す / 狙われない)
      case 44: 
        name = "ペインサイクル"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (与ダメージ分 HP 回復)
      case 45: 
        name = "ヴォイド・ショック"; tpCost = 45; power = 500; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高確率で状態異常)

      // --- ガーディアン (who=1) ---
      case 46: 
        name = "プロテクト"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 DEF 1.5倍)
      case 47: 
        name = "シールドバッシュ"; tpCost = 10; power = 80; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (威力20 + 確率でスタン)

      // --- フォートレス (who=6) ---
      case 48: 
        name = "ウォール"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 DEF 1.2倍)
      case 49: 
        name = "挑発"; tpCost = 10; power = 0; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (敵の攻撃を自分に集める)

      // --- キャッスル・ウォール (who=11) ---
      case 50: 
        name = "リフレクト"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (物理攻撃を一度だけ反射)
      case 51: 
        name = "ガーディアン魂"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (DEF 2倍、SPD 0.5倍)

      // --- バスティオン (who=17) ---
      case 52: 
        name = "鉄壁"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (3ターン DEF 2.5倍)
      case 53: 
        name = "カウンター"; tpCost = 5; power = 100; 
        type = EffectType::SPECIAL; scope = TargetScope::SELF; 
        break; // (受けたダメージを反撃)

      // --- イージス (who=18) ---
      case 54: 
        name = "リペア"; tpCost = 5; power = 60; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 回復)
      case 55: 
        name = "フォースフィールド"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 ダメージ無効 1回)

      // --- アダマンタイト (who=26) ---
      case 56: 
        name = "アダマン・ボディ"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (物理ダメージを 1 に)
      case 57: 
        name = "自己修復"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (HPリジェネ(大))
      case 58: 
        name = "ラストスタンド"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (HP1で耐える)
      case 59: 
        name = "ギガンティック・ウォール"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 DEF 2倍)

      // --- インヴィンシブル (who=27) ---
      case 60: 
        name = "超回復"; tpCost = 0; power = 500; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (HP大回復)
      case 61: 
        name = "オートリバイブ"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自動蘇生 1回)
      case 62: 
        name = "最大HP上昇"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (戦闘中 MaxHP 1.5倍)
      case 63: 
        name = "無敵"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (1ターン ダメージ 0)

      // --- ディヴァイン・シールド (who=28) ---
      case 64: 
        name = "全体バリア"; tpCost = 15; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 ダメージ無効 1回)
      case 65: 
        name = "旋律反射"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (旋律(魔法)反射)
      case 66: 
        name = "キュア・オール"; tpCost = 20; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 状態異常回復)
      case 67: 
        name = "ディバイン・ウォール"; tpCost = 20; power = 0;
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (効果: 全ダメージ軽減)
      
      // --- リジェネレーター (who=29) ---
      case 68: 
        name = "全体リペア"; tpCost = 10; power = 1000; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 回復)
      case 69: 
        name = "リザレクション"; tpCost = 0; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 蘇生)
      case 70: 
        name = "TPリジェネ"; tpCost = 0; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のTPを徐々に回復)
      case 71: 
        name = "フル・キュア"; tpCost = 10; power = 9999; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 HP全回復)

      // --- クリッパー (who=41) ---
      case 72: 
        name = "カッティング"; tpCost = 7; power = 80; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break;
      case 73: 
        name = "クイックステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 SPD バフ)

      // --- ブレード・ダンサー (who=46) ---
      case 74: 
        name = "ツインエッジ"; tpCost = 10; power = 60; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (35ダメージ x 2回)
      case 75: 
        name = "エアリアルダンス"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 AGI バフ)

      // --- サイクロン・ダンサー (who=51) ---
      case 76: 
        name = "スパイラルエッジ"; tpCost = 15; power = 100; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (70ダメージ x 2回)
      case 77: 
        name = "エアステップ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (味方単体 SPD/AGI バフ)

      // --- テンペスト (who=58) ---
      case 78: 
        name = "ゲイルストライク"; tpCost = 5; power = 140; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (風属性)
      case 79: 
        name = "ラピッドムーブ"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 SPD バフ)

      // --- シルフィード (who=69) ---
      case 80: 
        name = "インビジブルエッジ"; tpCost = 10; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (回避無視)
      case 81: 
        name = "ウインドウォール"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (味方全体 回避率UP)
      case 82: 
        name = "ストームブリンガー"; tpCost = 10; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::ALL_ENEMIES; 
        break; // (全体 風属性攻撃)
      case 83: 
        name = "精霊の舞"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のTP徐々に回復)

      // --- アクセラレーター (who=70) ---
      case 84: 
        name = "マッハブレイド"; tpCost = 13; power = 400; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制)
      case 85: 
        name = "オーバークロック"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 2回行動)
      case 86: 
        name = "リミットブレイク"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身のATK/SPD 最大まで上昇)
      case 87: 
        name = "タービュランス"; tpCost = 30; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::ALL_ENEMIES; 
        break; // (全体 風属性 + SPDダウン)

      // --- メンダー (who=42) ---
      case 88: 
        name = "リペア"; tpCost = 5; power = 50; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復)
      case 89: 
        name = "キュア"; tpCost = 5; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 状態異常回復)

      // --- (第1進化: who=47) ---
      case 90: 
        name = "リペア・プラス"; tpCost = 10; power = 150; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復(中))
      case 91: 
        name = "リフレッシュ"; tpCost = 5; power = 0; 
        type = EffectType::CURE_STATUS; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 弱体解除)

      // --- (第2進化: who=52) ---
      case 92: 
        name = "エリア・リペア"; tpCost = 15; power = 100; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP回復(小))
      case 93: 
        name = "リザレクション"; tpCost = 10; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 蘇生(HP中))

      // --- (第3進化: who=59) ---
      case 94: 
        name = "フル・リペア"; tpCost = 15; power = 500; 
        type = EffectType::HEAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP回復(大))
      case 95: 
        name = "オートリペア"; tpCost = 10; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 HP自動回復)

      // --- (最終進化A: who=71) ---
      case 96: 
        name = "エリア・リペア・プラス"; tpCost = 25; power = 250; 
        type = EffectType::HEAL; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP回復(中))
      case 97: 
        name = "リジェネフィールド"; tpCost = 20; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 HP自動回復)
      case 98: 
        name = "アセンション"; tpCost = 60; power = 0; 
        type = EffectType::REVIVE; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 蘇生(HP全快))
      case 99: 
        name = "ホーリー・ウォール"; tpCost = 20; power = 0;
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 状態異常無効 1回)

      // --- (最終進化B: who=72) ---
      case 100: 
        name = "TPチャージ"; tpCost = 10; power = 300; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ALLY; 
        break; // (単体 TP回復)
      case 101: 
        name = "ディスペル"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 強化解除)
      case 102: 
        name = "TPリジェネ"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::SELF; 
        break; // (自身 TP自動回復)
      case 103: 
        name = "オラクルフィールド"; tpCost = 5; power = 0; 
        type = EffectType::BUFF; scope = TargetScope::ALL_ALLIES; 
        break; // (全体 TP徐々に回復)

      // --- ジャマー (who=43) ---
      case 104: 
        name = "スロウ"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 SPD ダウン)
      case 105: 
        name = "ポイズン"; tpCost = 5; power = 50; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (ダメージ + 確率で毒)

      // --- (第1進化: who=48) ---
      case 106: 
        name = "パワーダウン"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 ATK ダウン)
      case 107: 
        name = "ガードダウン"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 DEF ダウン)

      // --- (第2進化: who=53) ---
      case 108: 
        name = "エリア・スロウ"; tpCost = 10; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 SPD ダウン)
      case 109: 
        name = "サイレンス"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 確率で沈黙)

      // --- (第3進化: who=60) ---
      case 110: 
        name = "デッドリーポイズン"; tpCost = 10; power = 400; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (ダメージ + 確率で猛毒)
      case 111: 
        name = "TPドレイン"; tpCost = 5; power = 380; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 TP吸収)

      // --- (最終進化A: who=73) ---
      case 112: 
        name = "エリア・ブレイク"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 ATK/DEF ダウン)
      case 113: 
        name = "カオスフィールド"; tpCost = 20; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 確率で混乱)
      case 114: 
        name = "ディザスター"; tpCost = 30; power = 1500; 
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY; 
        break; // (無属性 + 全状態異常付与)
      case 115: 
        name = "ペイン・イーター"; tpCost = 30; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (状態異常の敵に特効)

      // --- (最終進化B: who=74) ---
      case 116: 
        name = "ロックダウン"; tpCost = 0; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 確率で行動不能)
      case 117: 
        name = "バニッシュ"; tpCost = 5; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵単体 強化全解除)
      case 118: 
        name = "ワールド・ダウン"; tpCost = 15; power = 0; 
        type = EffectType::DEBUFF; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体 全能力ダウン)
      case 119: 
        name = "エナジー・バーン"; tpCost = 50; power = 300; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (敵のTPが多いほど高威力)

      // --- はぐれノイズ (仲間用) (who=75) ---
      case 120: 
        name = "ノイズヒット"; tpCost = 0; power = 90; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (シンプルな攻撃)
      case 121: 
        name = "ソニックダガー"; tpCost = 0; power = 120; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (必ず先制攻撃)
      case 122:
        name = "かく乱"; tpCost = 0; power = 90;
        type = EffectType::SPECIAL; scope = TargetScope::SINGLE_ENEMY;
        break; // (攻撃 + 確率で混乱)
      case 123: 
        name = "ファントムラッシュ"; tpCost = 0; power = 130; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::RANDOM_ENEMY; 
        break; // (40ダメージ x 2-4回ランダム攻撃)

      // --- はぐれキング (仲間用) (who=76) ---
      case 124: 
        name = "キングスピア"; tpCost = 0; power = 200; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (防御無視攻撃)
      case 125: 
        name = "ノイズストーム"; tpCost = 0; power = 150; 
        type = EffectType::DAMAGE; scope = TargetScope::ALL_ENEMIES; 
        break; // (敵全体攻撃)
      case 126: 
        name = "ロイヤルチャージ"; tpCost = 0; power = 1000; 
        type = EffectType::DAMAGE; scope = TargetScope::SINGLE_ENEMY; 
        break; // (高威力だが次のターン行動不可)
      case 127: 
        name = "ラッキーセブン"; tpCost = 0; power = 777; 
        type = EffectType::MULTI_HIT_DAMAGE; scope = TargetScope::RANDOM_ENEMY; 
        break; // (77ダメージ x 1-7回ランダム攻撃)

      default:
        name = "？？？";
        tpCost = 0;
        power = 0;
        type = EffectType::NONE; scope = TargetScope::SINGLE_ENEMY; 
        break;
    }
    // メンダー系 (88-103) は全て Sense 依存
    if (id >= 88 && id <= 103) {
        dependence = StatDependence::SEN;
    }
    // ジャマー系 (104-119) も全て Sense 依存
    if (id >= 104 && id <= 119) {
        dependence = StatDependence::SEN;
    }
    // 主人公の攻撃スキル (1-3, 12)
    if (id <= 3 || id == 12) {
        dependence = StatDependence::SEN;
    }
    // はぐれ系 (125 ノイズストーム, 126 ロイヤルチャージ) は魔法っぽい？
    if (id == 125 || id == 126) {
        dependence = StatDependence::SEN;
    }
    // スキル設定がすべて確定した後にカテゴリを決定
    bool isOffensive =
        type == EffectType::DAMAGE ||
        type == EffectType::MULTI_HIT_DAMAGE ||
        (type == EffectType::SPECIAL &&
        power > 0 &&
        (scope == TargetScope::SINGLE_ENEMY ||
          scope == TargetScope::ALL_ENEMIES ||
          scope == TargetScope::RANDOM_ENEMY));

    if (!isOffensive) {
        category = Category::OTHER;
    }
    else if (dependence == StatDependence::SEN) {
        category = Category::MELODY;
    }
    else {
        category = Category::PHYSICAL;
    }
  return skill;
}
