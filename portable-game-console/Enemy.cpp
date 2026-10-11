#include "Enemy.h"
#include "SkillLearnsetCatalog.h"

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
        break;

      // 20階ボス
      case 81: 
        status = new Status(1, 81);
        break;

      // 30階ボス
      case 82: 
        status = new Status(1, 82);
        // 状態異常3つ
        // 攻撃スキル
        break;

      // 40階ボス
      case 83: 
        status = new Status(1, 83);
        break;

      // 50階 ラスボス
      case 84: 
        status = new Status(1, 84);
        break;

      default: // 未定義ID
        who = 0; 
        status = new Status(3, who);
        break;
    }
    const SkillLearnsetCatalog::InitialSkills initialSkills =
        SkillLearnsetCatalog::getInitialSkills(who);
    for (int i = 0; i < initialSkills.count; i++) {
      status->forceLearnSkill(initialSkills.ids[i]);
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
