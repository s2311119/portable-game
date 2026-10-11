#include "Enemy.h"

#include <Arduino.h>
#include "Status.h"
#include "EnemyCatalog.h"

Enemy::Enemy(int w, int floor){
    who = w;
    int level = floor + random(0, 3);
    if (level < 1) level = 1;
    
    // who ID に応じて敵のデータを変える
    switch (who) {
      // Enemy クラスのコンストラクタ内 switch (who)

      // --- 1. 基本5体 (Lv 3-5) ---
      case 0: // 攻撃型
        status = new Status(level, who);
        break;
      case 1: // 防御型
        status = new Status(level, who);
        break;
      case 2: // 素早さ型
        status = new Status(level, who);
        break;
      case 3: // 回復型
        status = new Status(level, who);
        break;
      case 4: // からめ手型
        status = new Status(level, who);
        break;

      // --- 2. 1段階目進化 (Lv 10-12) ---
      case 5: // (ストライカー系)
        status = new Status(level, who);
        break;
      case 6: // (ガーディアン系)
        status = new Status(level, who);
        break;
      case 7: // (クリッパー系)
        status = new Status(level, who);
        break;
      case 8: // (メンダー系)
        status = new Status(level, who);
        break;
      case 9: // (ジャマー系)
        status = new Status(level, who);
        break;

      // --- 3. 2段階目進化 (Lv 20-22) ---
      case 10: // (攻撃型)
        status = new Status(level, who);
        break;
      case 11: // (防御型)
        status = new Status(level, who);
        break;
      case 12: // (素早さ型)
        status = new Status(level, who);
        break;
      case 13: // (回復/旋律型)
        status = new Status(level, who);
        break;
      case 14: // (妨害/幸運型)
        status = new Status(level, who);
        break;

      // --- 4. 3段階目進化 (Lv 30-32) ---
      // 進化の歯車をドロップする設定
      
      case 15: // ヴァンガード (攻撃の歯車)
         status = new Status(level, who);    break;
      case 16: // スワッシュバックラー (赤の歯車)
         status = new Status(level, who);    break;
      case 17: // バスティオン (防御の歯車)
         status = new Status(level, who);    break;
      case 18: // イージス (青の歯車)
          status = new Status(level, who);   break;
      case 19: // テンペスト (速度の歯車)
          status = new Status(level, who);   break;
      case 20: // マエストロ (回復の歯車)
          status = new Status(level, who);   break;
      case 21: // ディスコード (感応の歯車)
          status = new Status(level, who);   break;

      // --- 5. 4段階目（最終）進化 (Lv 40-42) ---
      // ※ さらに上位の歯車や、レアアイテムを落とす
      
      case 22: // オーバーロード (赤の歯車)
          status = new Status(level, who);   break;
      case 23: // デストロイヤー (紅の歯車)
          status = new Status(level, who);   break;
      case 24: // ゼロ・ブレード (紅の歯車)
          status = new Status(level, who);   break;
      case 25: // ヴォイド・ストーカー (紅の歯車)
          status = new Status(level, who);   break;
        
      case 26: // アダマンタイト (青の歯車)
          status = new Status(level, who);   break;
      case 27: // インヴィンシブル (蒼の歯車)
          status = new Status(level, who);  break;
      case 28: // ディヴァイン・シールド (蒼の歯車)
          status = new Status(level, who);   break;
      case 29: // リジェネレーター (蒼の歯車)
          status = new Status(level, who);   break;

      case 30: // シルフィード (速度の歯車)
          status = new Status(level, who);   break;
      case 31: // アクセラレーター (銀の歯車)
          status = new Status(level, who);   break;
        
      case 32: // アーク・ハーモニスト (回復の歯車)
          status = new Status(level, who);  break;
      case 33: // シンフォニア (リペアキット大)
          status = new Status(level, who);   break;
        
      case 34: // パンデモニウム (感応の歯車)
          status = new Status(level, who);  break;
      case 35: // ジョーカー (謎のパーツ)
          status = new Status(level, who);   break;

      // --- 6. 特殊な敵 ---
      case 36: // はぐれノイズ (XP特化)
        status = new Status(5, who);
        break;

      case 37: // ノイズ・キング (XP特化)
        status = new Status(8, who);
        break;

      case 80: 
        status = new Status(1, 80);
        status->forceLearnSkill(21);
        status->forceLearnSkill(106);
        status->forceLearnSkill(47);
        break;

      // 20階ボス
      case 81: 
        status = new Status(1, 81);
        status->forceLearnSkill(52);
        status->forceLearnSkill(34);
        status->forceLearnSkill(87);
        break;

      // 30階ボス
      case 82: 
        status = new Status(1, 82);
        // 状態異常3つ
        status->forceLearnSkill(105); // ポイズン (毒)
        status->forceLearnSkill(45);  // ヴォイド・ショック (麻痺)
        status->forceLearnSkill(113); // カオスフィールド (混乱)
        // 攻撃スキル
        status->forceLearnSkill(42);  // アサシネイト (確率で即死級ダメージ)
        break;

      // 40階ボス
      case 83: 
        status = new Status(1, 83);
        status->forceLearnSkill(73);  // クイックステップ (素早さUP)
        status->forceLearnSkill(39);  // ミラーズエッジ (回避率2倍)
        status->forceLearnSkill(109); // サイレンス (沈黙与) -> 詠唱を封じる
        status->forceLearnSkill(25);  // ソードダンス (攻撃・素早さUP)
        status->forceLearnSkill(123); // ファントムラッシュ (ランダム2~5回攻撃)
        status->forceLearnSkill(80);  // インビジブルエッジ (回避無視の必中攻撃)
        break;

      // 50階 ラスボス
      case 84: 
        status = new Status(1, 84);
        status->forceLearnSkill(86);  // リミットブレイク (全能力最大強化)
        status->forceLearnSkill(118); // ワールド・ダウン (敵全体全能力DOWN)
        status->forceLearnSkill(114); // ディザスター (全状態異常付与)
        status->forceLearnSkill(57);  // 自己修復 (HPリジェネ)
        status->forceLearnSkill(64);  // 全体バリア (ダメージ1回無効)
        status->forceLearnSkill(65);  // 旋律反射 (魔法反射)
        status->forceLearnSkill(53);  // カウンター (物理反撃)
        status->forceLearnSkill(111); // TPドレイン (TP吸収)
        status->forceLearnSkill(127); // ラッキーセブン (ランダム1~8回攻撃)
        status->forceLearnSkill(126); // ロイヤルチャージ (溜めて次のターン超火力)
        break;

      default: // 未定義ID
        who = 0; 
        status = new Status(3, who);
        break;
    }
    const EnemyData& data = EnemyCatalog::get(w);
    name = data.name;
    dropItemId = data.dropItemId;
    dropRatePercent = data.dropRatePercent;
    xpYield = EnemyCatalog::xpYieldFor(w, level);
}

Enemy::~Enemy(){
    delete status;
    status = nullptr;
}
