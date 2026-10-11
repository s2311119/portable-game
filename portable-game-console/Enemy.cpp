#include "Enemy.h"

#include <Arduino.h>
#include "Status.h"

Enemy::Enemy(int w, int floor){
    who = w;
    int level = floor + random(0, 3);
    if (level < 1) level = 1;
    
    // who ID に応じて敵のデータを変える
    switch (who) {
      // Enemy クラスのコンストラクタ内 switch (who)

      // --- 1. 基本5体 (Lv 3-5) ---
      case 0: // 攻撃型
        name = "ストライカー";
        status = new Status(level, who);
        dropItemId = 1; // 鉄くず (進化素材)
        dropRatePercent = 20; 
        xpYield = 5 + level;
        break;
      case 1: // 防御型
        name = "ガーディアン";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 20;
        xpYield = 5 + level;
        break;
      case 2: // 素早さ型
        name = "クリッパー";
        status = new Status(level, who);
        dropItemId = 1;
        dropRatePercent = 20;
        xpYield = 5 + level;
        break;
      case 3: // 回復型
        name = "メンダー";
        status = new Status(level, who);
        dropItemId = 100; // リペアキット
        dropRatePercent = 30;
        xpYield = 5 + level;
        break;
      case 4: // からめ手型
        name = "ジャマー";
        status = new Status(level, who);
        dropItemId = 2; // 銅の歯車 (換金)
        dropRatePercent = 30;
        xpYield = 5 + level;
        break;

      // --- 2. 1段階目進化 (Lv 10-12) ---
      case 5: // (ストライカー系)
        name = "ギア・ソルジャー";
        status = new Status(level, who);
        dropItemId = 1; // 鉄くず
        dropRatePercent = 30;
        xpYield = 20 + level * 2;
        break;
      case 6: // (ガーディアン系)
        name = "フォートレス";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 30;
        xpYield = 22 + level * 2;
        break;
      case 7: // (クリッパー系)
        name = "ブレード・ダンサー";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 30;
        xpYield = 18 + level * 2;
        break;
      case 8: // (メンダー系)
        name = "ハーモニスト";
        status = new Status(level, who);
        dropItemId = 103; // エネルギー缶
        dropRatePercent = 25;
        xpYield = 20 + level * 2;
        break;
      case 9: // (ジャマー系)
        name = "ディスアレンジャー";
        status = new Status(level, who);
        dropItemId = 2; // 銅の歯車
        dropRatePercent = 40;
        xpYield = 19 + level * 2;
        break;

      // --- 3. 2段階目進化 (Lv 20-22) ---
      case 10: // (攻撃型)
        name = "ギア・ナイト";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 50;
        break;
      case 11: // (防御型)
        name = "キャッスル・ウォール";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 55;
        break;
      case 12: // (素早さ型)
        name = "サイクロン・ダンサー";
        status = new Status(level, who);
        dropItemId = 1; 
        dropRatePercent = 40;
        xpYield = 48;
        break;
      case 13: // (回復/旋律型)
        name = "コンダクター";
        status = new Status(level, who);
        dropItemId = 101; // リペアキット中
        dropRatePercent = 30;
        xpYield = 52;
        break;
      case 14: // (妨害/幸運型)
        name = "カオス・ジャマー";
        status = new Status(level, who);
        dropItemId = 104; // 万能オイル
        dropRatePercent = 30;
        xpYield = 49;
        break;

      // --- 4. 3段階目進化 (Lv 30-32) ---
      // 進化の歯車をドロップする設定
      
      case 15: // ヴァンガード (攻撃の歯車)
        name = "ヴァンガード"; status = new Status(level, who); dropItemId = 10; dropRatePercent = 20; xpYield = 150; break;
      case 16: // スワッシュバックラー (赤の歯車)
        name = "スワッシュバックラー"; status = new Status(level, who); dropItemId = 11; dropRatePercent = 20; xpYield = 150; break;
      case 17: // バスティオン (防御の歯車)
        name = "バスティオン"; status = new Status(level, who); dropItemId = 13; dropRatePercent = 20; xpYield = 160; break;
      case 18: // イージス (青の歯車)
        name = "イージス"; dropItemId = 14; status = new Status(level, who); dropRatePercent = 20; xpYield = 170; break;
      case 19: // テンペスト (速度の歯車)
        name = "テンペスト"; dropItemId = 16; status = new Status(level, who); dropRatePercent = 20; xpYield = 145; break;
      case 20: // マエストロ (回復の歯車)
        name = "マエストロ"; dropItemId = 17; status = new Status(level, who); dropRatePercent = 20; xpYield = 155; break;
      case 21: // ディスコード (感応の歯車)
        name = "ディスコード"; dropItemId = 18; status = new Status(level, who); dropRatePercent = 20; xpYield = 150; break;

      // --- 5. 4段階目（最終）進化 (Lv 40-42) ---
      // ※ さらに上位の歯車や、レアアイテムを落とす
      
      case 22: // オーバーロード (赤の歯車)
        name = "オーバーロード"; dropItemId = 11; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 23: // デストロイヤー (紅の歯車)
        name = "デストロイヤー"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 24: // ゼロ・ブレード (紅の歯車)
        name = "ゼロ・ブレード"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
      case 25: // ヴォイド・ストーカー (紅の歯車)
        name = "ヴォイド・ストーカー"; dropItemId = 12; status = new Status(level, who); dropRatePercent = 15; xpYield = 400; break;
        
      case 26: // アダマンタイト (青の歯車)
        name = "アダマンタイト"; dropItemId = 14; status = new Status(level, who); dropRatePercent = 15; xpYield = 420; break;
      case 27: // インヴィンシブル (蒼の歯車)
        name = "インヴィンシブル"; dropItemId = 15; status = new Status(level, who);dropRatePercent = 15; xpYield = 450; break;
      case 28: // ディヴァイン・シールド (蒼の歯車)
        name = "ディヴァイン・シールド"; dropItemId = 15; status = new Status(level, who); dropRatePercent = 15; xpYield = 430; break;
      case 29: // リジェネレーター (蒼の歯車)
        name = "リジェネレーター"; dropItemId = 15; status = new Status(level, who); dropRatePercent = 15; xpYield = 430; break;

      case 30: // シルフィード (速度の歯車)
        name = "シルフィード"; dropItemId = 16; status = new Status(level, who); dropRatePercent = 20; xpYield = 390; break;
      case 31: // アクセラレーター (銀の歯車)
        name = "アクセラレーター"; dropItemId = 3; status = new Status(level, who); dropRatePercent = 20; xpYield = 390; break;
        
      case 32: // アーク・ハーモニスト (回復の歯車)
        name = "アーク・ハーモニスト"; dropItemId = 17; status = new Status(level, who);dropRatePercent = 20; xpYield = 410; break;
      case 33: // シンフォニア (リペアキット大)
        name = "シンフォニア"; dropItemId = 102; status = new Status(level, who); dropRatePercent = 20; xpYield = 410; break;
        
      case 34: // パンデモニウム (感応の歯車)
        name = "パンデモニウム"; dropItemId =18; status = new Status(level, who);dropRatePercent = 30; xpYield = 400; break;
      case 35: // ジョーカー (謎のパーツ)
        name = "ジョーカー"; dropItemId = 5; status = new Status(level, who); dropRatePercent = 30; xpYield = 400; break;

      // --- 6. 特殊な敵 ---
      case 36: // はぐれノイズ (XP特化)
        name = "はぐれノイズ";
        status = new Status(5, who);
        dropItemId = 4; // 金の歯車
        dropRatePercent = 100; // 確定ドロップ
        xpYield = 10000; 
        break;

      case 37: // ノイズ・キング (XP特化)
        name = "ノイズ・キング";
        status = new Status(8, who);
        dropItemId = 504; // ラッキーコイン (アクセ)
        dropRatePercent = 10; // 激レア
        xpYield = 50000;
        break;

      case 80: 
        name = "スプラウト・ギア"; // 名前
        status = new Status(1, 80);
        status->setBossStats(500, 100, 40, 20, 15, 10, 10); // HP, TP, ATK, DEF, SPD, SEN, LCK
        status->forceLearnSkill(21);
        status->forceLearnSkill(106);
        status->forceLearnSkill(47);
        dropItemId = 500; // パワーリング
        dropRatePercent = 100; 
        xpYield = 500;
        break;

      // 20階ボス
      case 81: 
        name = "ソレイユ";
        status = new Status(1, 81);
        status->setBossStats(1200, 200, 70, 60, 10, 20, 20);
        status->forceLearnSkill(52);
        status->forceLearnSkill(34);
        status->forceLearnSkill(87);
        dropItemId = 501; // ガードリング
        dropRatePercent = 100;
        xpYield = 1500;
        break;

      // 30階ボス
      case 82: 
        name = "ハーベスト・スティンガー";
        status = new Status(1, 82);
        status->setBossStats(3000, 500, 120, 80, 60, 50, 40);
        // 状態異常3つ
        status->forceLearnSkill(105); // ポイズン (毒)
        status->forceLearnSkill(45);  // ヴォイド・ショック (麻痺)
        status->forceLearnSkill(113); // カオスフィールド (混乱)
        // 攻撃スキル
        status->forceLearnSkill(42);  // アサシネイト (確率で即死級ダメージ)
        dropItemId = 599; // トリニティ・コア
        dropRatePercent = 100; // 50%
        xpYield = 4000;
        break;

      // 40階ボス
      case 83: 
        name = "雪影";
        status = new Status(1, 83);
        status->setBossStats(8000, 999, 200, 150, 80, 100, 50);
        status->forceLearnSkill(73);  // クイックステップ (素早さUP)
        status->forceLearnSkill(39);  // ミラーズエッジ (回避率2倍)
        status->forceLearnSkill(109); // サイレンス (沈黙与) -> 詠唱を封じる
        status->forceLearnSkill(25);  // ソードダンス (攻撃・素早さUP)
        status->forceLearnSkill(123); // ファントムラッシュ (ランダム2~5回攻撃)
        status->forceLearnSkill(80);  // インビジブルエッジ (回避無視の必中攻撃)
        dropItemId = 399; // 創世のタクト
        dropRatePercent = 100;
        xpYield = 10000;
        break;

      // 50階 ラスボス
      case 84: 
        name = "デウス・エクス・マキナ";
        status = new Status(1, 84);
        status->setBossStats(20000, 9999, 300, 200, 120, 150, 100);
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
        dropItemId = 0; 
        dropRatePercent = 0;
        xpYield = 0;
        break;

      default: // 未定義ID
        who = 0; 
        name = "ストライカー";
        status = new Status(3, who);
        dropItemId = 1;
        dropRatePercent = 20;
        xpYield = 5;
        break;
    }
}

Enemy::~Enemy(){
    delete status;
    status = nullptr;
}
