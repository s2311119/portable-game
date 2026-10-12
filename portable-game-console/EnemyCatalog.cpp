#include "EnemyCatalog.h"

namespace {

const EnemyData enemyDefinitions[] = {
    {0, "ストライカー", 1, 20},
    {1, "ガーディアン", 1, 20},
    {2, "クリッパー", 1, 20},
    {3, "メンダー", 100, 30},
    {4, "ジャマー", 2, 30},
    {5, "ギア・ソルジャー", 1, 30},
    {6, "フォートレス", 1, 30},
    {7, "ブレード・ダンサー", 1, 30},
    {8, "ハーモニスト", 103, 25},
    {9, "ディスアレンジャー", 2, 40},
    {10, "ギア・ナイト", 1, 40},
    {11, "キャッスル・ウォール", 1, 40},
    {12, "サイクロン・ダンサー", 1, 40},
    {13, "コンダクター", 101, 30},
    {14, "カオス・ジャマー", 104, 30},
    {15, "ヴァンガード", 10, 20},
    {16, "スワッシュバックラー", 11, 20},
    {17, "バスティオン", 13, 20},
    {18, "イージス", 14, 20},
    {19, "テンペスト", 16, 20},
    {20, "マエストロ", 17, 20},
    {21, "ディスコード", 18, 20},
    {22, "オーバーロード", 11, 15},
    {23, "デストロイヤー", 12, 15},
    {24, "ゼロ・ブレード", 12, 15},
    {25, "ヴォイド・ストーカー", 12, 15},
    {26, "アダマンタイト", 14, 15},
    {27, "インヴィンシブル", 15, 15},
    {28, "ディヴァイン・シールド", 15, 15},
    {29, "リジェネレーター", 15, 15},
    {30, "シルフィード", 16, 20},
    {31, "アクセラレーター", 3, 20},
    {32, "アーク・ハーモニスト", 17, 20},
    {33, "シンフォニア", 102, 20},
    {34, "パンデモニウム", 18, 30},
    {35, "ジョーカー", 5, 30},
    {36, "はぐれノイズ", 4, 100},
    {37, "ノイズ・キング", 504, 10},
    {80, "スプラウト・ギア", 500, 100},
    {81, "ソレイユ", 501, 100},
    {82, "ハーベスト・スティンガー", 599, 100},
    {83, "雪影", 399, 100},
    {84, "デウス・エクス・マキナ", 0, 0},
};

} // namespace

const EnemyData& EnemyCatalog::get(int id) {
    for (const EnemyData& data : enemyDefinitions) {
        if (data.id == id) return data;
    }
    return enemyDefinitions[0];
}

int EnemyCatalog::xpYieldFor(int id, int level) {
    switch (id) {
        case 0: return 5 + level;
        case 1: return 5 + level;
        case 2: return 5 + level;
        case 3: return 5 + level;
        case 4: return 5 + level;
        case 5: return 20 + level * 2;
        case 6: return 22 + level * 2;
        case 7: return 18 + level * 2;
        case 8: return 20 + level * 2;
        case 9: return 19 + level * 2;
        case 10: return 50;
        case 11: return 55;
        case 12: return 48;
        case 13: return 52;
        case 14: return 49;
        case 15: return 150;
        case 16: return 150;
        case 17: return 160;
        case 18: return 170;
        case 19: return 145;
        case 20: return 155;
        case 21: return 150;
        case 22: return 400;
        case 23: return 400;
        case 24: return 400;
        case 25: return 400;
        case 26: return 420;
        case 27: return 450;
        case 28: return 430;
        case 29: return 430;
        case 30: return 390;
        case 31: return 390;
        case 32: return 410;
        case 33: return 410;
        case 34: return 400;
        case 35: return 400;
        case 36: return 10000;
        case 37: return 50000;
        case 80: return 500;
        case 81: return 1500;
        case 82: return 4000;
        case 83: return 10000;
        case 84: return 0;
        default: return 5;
    }
}
