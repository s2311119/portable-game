#include "CharacterCatalog.h"

namespace {

int decide_MaxHP(int l, int who){
    if(who == 0) return l * 3 + 12; // 0:ストライカー (並)
    if(who == 1) return l * 5 + 15; // 1:ガーディアン (高い)
    if(who == 2) return l * 2 + 8;  // 2:クリッパー (低い)
    if(who == 3) return l * 3 + 12; // 3:メンダー (並)
    if(who == 4) return l * 2 + 10; // 4:ジャマー (低い)

    // 1段階目進化
    if(who == 5) return l * 4 + 15; // 5:ギア・ソルジャー (並+)
    if(who == 6) return l * 6 + 20; // 6:フォートレス (高い+)
    if(who == 7) return l * 3 + 10; // 7:ブレード・ダンサー (低い+)
    if(who == 8) return l * 4 + 14; // 8:ハーモニスト (並+)
    if(who == 9) return l * 3 + 12; // 9:ディスアレンジャー (低い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 5 + 20; // 10:ギア・ナイト (並++)
    if(who == 11) return l * 7 + 25; // 11:キャッスル・ウォール (高い++)
    if(who == 12) return l * 4 + 12; // 12:サイクロン・ダンサー (低い++)
    if(who == 13) return l * 5 + 16; // 13:コンダクター (並++)
    if(who == 14) return l * 4 + 14; // 14:カオス・ジャマー (低い++)
    // 3段階目進化 (15-21)
    if(who == 15) return l * 6 + 25; // 15:ヴァンガード (純ATK)
    if(who == 16) return l * 5 + 20; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 17) return l * 8 + 30; // 17:バスティオン (純DEF)
    if(who == 18) return l * 10 + 40; // 18:イージス (DEF+HP)
    if(who == 19) return l * 5 + 15; // 19:テンペスト (SPD)
    if(who == 20) return l * 6 + 18; // 20:マエストロ (SEN)
    if(who == 21) return l * 5 + 16; // 21:ディスコード (LCK)
    // 4段階目 (22-35)
    if(who == 22) return l * 8 + 30;  // 22:オーバーロード (バランス)
    if(who == 23) return l * 6 + 25;  // 23:デストロイヤー (ATK特化)
    if(who == 24) return l * 6 + 22;  // 24:ゼロ・ブレード (ATK/SPD)
    if(who == 25) return l * 5 + 20;  // 25:ヴォイド・ストーカー (SPD/LCK)
    if(who == 26) return l * 9 + 35;  // 26:アダマンタイト (純DEF)
    if(who == 27) return l * 12 + 50; // 27:インヴィンシブル (DEF+HP)
    if(who == 28) return l * 10 + 40; // 28:ディヴァイン・シールド (DEF/SEN)
    if(who == 29) return l * 12 + 50; // 29:リジェネレーター (HP/SEN)
    if(who == 30) return l * 6 + 18;  // 30:シルフィード (SPD/LCK)
    if(who == 31) return l * 6 + 20;  // 31:アクセラレーター (SPD/ATK)
    if(who == 32) return l * 7 + 20;  // 32:アーク・ハーモニスト (純SEN)
    if(who == 33) return l * 8 + 22;  // 33:シンフォニア (万能SEN)
    if(who == 34) return l * 7 + 18;  // 34:パンデモニウム (LCK/SEN)
    if(who == 35) return l * 6 + 18;  // 35:ジョーカー (LCK/SPD)
    // 特殊 (36-37)
    if(who == 36) return 8;  // 36:はぐれノイズ (HP固定)
    if(who == 37) return 12; // 37:ノイズ・キング (HP固定)

    //主人公
    if(who == 38) return l * 3 + 20;

    //仲間
    if(who == 39) return l * 3 + 12;
    if(who == 40) return l * 5 + 15;
    if(who == 41) return l * 2 + 8;
    if(who == 42) return l * 3 + 12;
    if(who == 43) return l * 2 + 10;
    //1段階進化
    if(who == 44) return l * 4 + 15;
    if(who == 45) return l * 6 + 20;
    if(who == 46) return l * 3 + 10;
    if(who == 47) return l * 4 + 14;
    if(who == 48) return l * 3 + 12;
    // 2段階目進化 (10-14)
    if(who == 49) return l * 5 + 20; // 10:ギア・ナイト (並++)
    if(who == 50) return l * 7 + 25; // 11:キャッスル・ウォール (高い++)
    if(who == 51) return l * 4 + 12; // 12:サイクロン・ダンサー (低い++)
    if(who == 52) return l * 5 + 16; // 13:コンダクター (並++)
    if(who == 53) return l * 4 + 14; // 14:カオス・ジャマー (低い++)
    // 3段階目進化 (15-21)
    if(who == 54) return l * 6 + 25; // 15:ヴァンガード (純ATK)
    if(who == 55) return l * 5 + 20; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 56) return l * 8 + 30; // 17:バスティオン (純DEF)
    if(who == 57) return l * 10 + 40; // 18:イージス (DEF+HP)
    if(who == 58) return l * 5 + 15; // 19:テンペスト (SPD)
    if(who == 59) return l * 6 + 18; // 20:マエストロ (SEN)
    if(who == 60) return l * 5 + 16; // 21:ディスコード (LCK)
    // 4段階目 (22-35)
    if(who == 61) return l * 8 + 30;  // 22:オーバーロード (バランス)
    if(who == 62) return l * 6 + 25;  // 23:デストロイヤー (ATK特化)
    if(who == 63) return l * 6 + 22;  // 24:ゼロ・ブレード (ATK/SPD)
    if(who == 64) return l * 5 + 20;  // 25:ヴォイド・ストーカー (SPD/LCK)
    if(who == 65) return l * 9 + 35;  // 26:アダマンタイト (純DEF)
    if(who == 66) return l * 12 + 50; // 27:インヴィンシブル (DEF+HP)
    if(who == 67) return l * 10 + 40; // 28:ディヴァイン・シールド (DEF/SEN)
    if(who == 68) return l * 12 + 50; // 29:リジェネレーター (HP/SEN)
    if(who == 69) return l * 6 + 18;  // 30:シルフィード (SPD/LCK)
    if(who == 70) return l * 6 + 20;  // 31:アクセラレーター (SPD/ATK)
    if(who == 71) return l * 7 + 20;  // 32:アーク・ハーモニスト (純SEN)
    if(who == 72) return l * 8 + 22;  // 33:シンフォニア (万能SEN)
    if(who == 73) return l * 7 + 18;  // 34:パンデモニウム (LCK/SEN)
    if(who == 74) return l * 6 + 18;  // 35:ジョーカー (LCK/SPD)
    if(who == 80) return 500;
    if(who == 81) return 1200;
    if(who == 82) return 3000;
    if(who == 83) return 8000;
    if(who == 84) return 20000;
    // 特殊 (36-37)
    if(who == 36) return l + 8;  // 36:はぐれノイズ (HP固定)
    if(who == 37) return l * 2 + 12; // 37:ノイズ・キング (HP固定)
    return 0;
  }

int decide_MaxTP(int l, int who){
    if(who == 0) return l * 1 + 5;  // 0:ストライカー (低い)
    if(who == 1) return l * 1 + 5;  // 1:ガーディアン (低い)
    if(who == 2) return l * 2 + 10; // 2:クリッパー (並)
    if(who == 3) return l * 5 + 15; // 3:メンダー (高い)
    if(who == 4) return l * 4 + 10; // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 1 + 6;  // 5:ギア・ソルジャー (低い)
    if(who == 6) return l * 1 + 6;  // 6:フォートレス (低い)
    if(who == 7) return l * 2 + 12; // 7:ブレード・ダンサー (並)
    if(who == 8) return l * 6 + 18; // 8:ハーモニスト (高い+)
    if(who == 9) return l * 5 + 12; // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 7;  // 10:ギア・ナイト (低い)
    if(who == 11) return l * 1 + 7;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 2 + 15; // 12:サイクロン・ダンサー (並)
    if(who == 13) return l * 7 + 22; // 13:コンダクター (高い++)
    if(who == 14) return l * 6 + 15; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 1 + 8;  // 15:ヴァンガード
    if(who == 16) return l * 2 + 10; // 16:スワッシュバックラー
    if(who == 17) return l * 1 + 8;  // 17:バスティオン
    if(who == 18) return l * 4 + 20; // 18:イージス
    if(who == 19) return l * 3 + 18; // 19:テンペスト
    if(who == 20) return l * 9 + 30; // 20:マエストロ
    if(who == 21) return l * 8 + 25; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 2 + 10;  // 22:オーバーロード
    if(who == 23) return l * 1 + 8;   // 23:デストロイヤー
    if(who == 24) return l * 3 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 3 + 15;  // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 10;  // 26:アダマンタイト
    if(who == 27) return l * 2 + 10;  // 27:インヴィンシブル
    if(who == 28) return l * 6 + 25;  // 28:ディヴァイン・シールド
    if(who == 29) return l * 7 + 30;  // 29:リジェネレーター
    if(who == 30) return l * 4 + 20;  // 30:シルフィード
    if(who == 31) return l * 3 + 18;  // 31:アクセラレーター
    if(who == 32) return l * 12 + 40; // 32:アーク・ハーモニスト
    if(who == 33) return l * 10 + 35; // 33:シンフォニア
    if(who == 34) return l * 11 + 30; // 34:パンデモニウム
    if(who == 35) return l * 10 + 28; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return 0; // 36:はぐれノイズ (TPなし)
    if(who == 37) return 0; // 37:ノイズ・キング (TPなし)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 5;
    if(who == 40) return l * 1 + 5;  // 1:ガーディアン (低い)
    if(who == 41) return l * 2 + 10; // 2:クリッパー (並)
    if(who == 42) return l * 5 + 15; // 3:メンダー (高い)
    if(who == 43) return l * 4 + 10; // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 1 + 6;  // 5:ギア・ソルジャー (低い)
    if(who == 45) return l * 1 + 6;  // 6:フォートレス (低い)
    if(who == 46) return l * 2 + 12; // 7:ブレード・ダンサー (並)
    if(who == 47) return l * 6 + 18; // 8:ハーモニスト (高い+)
    if(who == 48) return l * 5 + 12; // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 7;  // 10:ギア・ナイト (低い)
    if(who == 50) return l * 1 + 7;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 2 + 15; // 12:サイクロン・ダンサー (並)
    if(who == 52) return l * 7 + 22; // 13:コンダクター (高い++)
    if(who == 53) return l * 6 + 15; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 1 + 8;  // 15:ヴァンガード
    if(who == 55) return l * 2 + 10; // 16:スワッシュバックラー
    if(who == 56) return l * 1 + 8;  // 17:バスティオン
    if(who == 57) return l * 4 + 20; // 18:イージス
    if(who == 58) return l * 3 + 18; // 19:テンペスト
    if(who == 59) return l * 9 + 30; // 20:マエストロ
    if(who == 60) return l * 8 + 25; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 2 + 10;  // 22:オーバーロード
    if(who == 62) return l * 1 + 8;   // 23:デストロイヤー
    if(who == 63) return l * 3 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 3 + 15;  // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 10;  // 26:アダマンタイト
    if(who == 66) return l * 2 + 10;  // 27:インヴィンシブル
    if(who == 67) return l * 6 + 25;  // 28:ディヴァイン・シールド
    if(who == 68) return l * 7 + 30;  // 29:リジェネレーター
    if(who == 69) return l * 4 + 20;  // 30:シルフィード
    if(who == 70) return l * 3 + 18;  // 31:アクセラレーター
    if(who == 71) return l * 12 + 40; // 32:アーク・ハーモニスト
    if(who == 72) return l * 10 + 35; // 33:シンフォニア
    if(who == 73) return l * 11 + 30; // 34:パンデモニウム
    if(who == 74) return l * 10 + 28; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return 0; // 36:はぐれノイズ (TPなし)
    if(who == 76) return 0; // 37:ノイズ・キング (TPなし)
    return 0;
  }

int decide_Attack(int l, int who){
    if(who == 0) return l * 5 + 5;  // 0:ストライカー (高い)
    if(who == 1) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 2) return l * 3 + 3;  // 2:クリッパー (並)
    if(who == 3) return l * 1 + 2;  // 3:メンダー (最低)
    if(who == 4) return l * 1 + 2;  // 4:ジャマー (最低)
    // 1段階目進化
    if(who == 5) return l * 6 + 8;  // 5:ギア・ソルジャー (高い+)
    if(who == 6) return l * 2 + 4;  // 6:フォートレス (低い)
    if(who == 7) return l * 4 + 4;  // 7:ブレード・ダンサー (並+)
    if(who == 8) return l * 1 + 2;  // 8:ハーモニスト (最低)
    if(who == 9) return l * 1 + 2;  // 9:ディスアレンジャー (最低)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 7 + 12; // 10:ギア・ナイト (高い++)
    if(who == 11) return l * 2 + 5;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 5 + 6;  // 12:サイクロン・ダンサー (並++)
    if(who == 13) return l * 1 + 2;  // 13:コンダクター (最低)
    if(who == 14) return l * 1 + 2;  // 14:カオス・ジャマー (最低)
    // 3段階目 (15-21)
    if(who == 15) return l * 9 + 15; // 15:ヴァンガード (純ATK)
    if(who == 16) return l * 8 + 13; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 17) return l * 2 + 6;  // 17:バスティオン
    if(who == 18) return l * 2 + 6;  // 18:イージス
    if(who == 19) return l * 6 + 8;  // 19:テンペスト
    if(who == 20) return l * 1 + 2;  // 20:マエストロ
    if(who == 21) return l * 1 + 2;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 10 + 20; // 22:オーバーロード
    if(who == 23) return l * 12 + 25; // 23:デストロイヤー
    if(who == 24) return l * 10 + 15; // 24:ゼロ・ブレード
    if(who == 25) return l * 8 + 10;  // 25:ヴォイド・ストーカー
    if(who == 26) return l * 3 + 8;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 8;   // 27:インヴィンシブル
    if(who == 28) return l * 4 + 8;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 3 + 7;   // 29:リジェネレーター
    if(who == 30) return l * 7 + 10;  // 30:シルフィード
    if(who == 31) return l * 9 + 12;  // 31:アクセラレーター
    if(who == 32) return l * 1 + 2;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 5 + 5;   // 33:シンフォニア
    if(who == 34) return l * 2 + 3;   // 34:パンデモニウム
    if(who == 35) return l * 6 + 6;   // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 1 + 1; // 36:はぐれノイズ (攻撃しない)
    if(who == 37) return l * 1 + 1; // 37:ノイズ・キング (攻撃しない)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 5 + 5;
    if(who == 40) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 41) return l * 3 + 3;  // 2:クリッパー (並)
    if(who == 42) return l * 1 + 2;  // 3:メンダー (最低)
    if(who == 43) return l * 1 + 2;  // 4:ジャマー (最低)
    // 1段階目進化
    if(who == 44) return l * 6 + 8;  // 5:ギア・ソルジャー (高い+)
    if(who == 45) return l * 2 + 4;  // 6:フォートレス (低い)
    if(who == 46) return l * 4 + 4;  // 7:ブレード・ダンサー (並+)
    if(who == 47) return l * 1 + 2;  // 8:ハーモニスト (最低)
    if(who == 48) return l * 1 + 2;  // 9:ディスアレンジャー (最低)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 7 + 12; // 10:ギア・ナイト (高い++)
    if(who == 50) return l * 2 + 5;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 5 + 6;  // 12:サイクロン・ダンサー (並++)
    if(who == 52) return l * 1 + 2;  // 13:コンダクター (最低)
    if(who == 53) return l * 1 + 2;  // 14:カオス・ジャマー (最低)
    // 3段階目 (15-21)
    if(who == 54) return l * 9 + 15; // 15:ヴァンガード (純ATK)
    if(who == 55) return l * 8 + 13; // 16:スワッシュバックラー (ATK+SPD)
    if(who == 56) return l * 2 + 6;  // 17:バスティオン
    if(who == 57) return l * 2 + 6;  // 18:イージス
    if(who == 58) return l * 6 + 8;  // 19:テンペスト
    if(who == 59) return l * 1 + 2;  // 20:マエストロ
    if(who == 60) return l * 1 + 2;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 10 + 20; // 22:オーバーロード
    if(who == 62) return l * 12 + 25; // 23:デストロイヤー
    if(who == 63) return l * 10 + 15; // 24:ゼロ・ブレード
    if(who == 64) return l * 8 + 10;  // 25:ヴォイド・ストーカー
    if(who == 65) return l * 3 + 8;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 8;   // 27:インヴィンシブル
    if(who == 67) return l * 4 + 8;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 3 + 7;   // 29:リジェネレーター
    if(who == 69) return l * 7 + 10;  // 30:シルフィード
    if(who == 70) return l * 9 + 12;  // 31:アクセラレーター
    if(who == 71) return l * 1 + 2;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 5 + 5;   // 33:シンフォニア
    if(who == 73) return l * 2 + 3;   // 34:パンデモニウム
    if(who == 74) return l * 6 + 6;   // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 1 + 1; // 36:はぐれノイズ (攻撃しない)
    if(who == 76) return l * 1 + 1; // 37:ノイズ・キング (攻撃しない)
    
    return 0;
  }

int decide_Defense(int l, int who){
    if(who == 0) return l * 1 + 3;  // 0:ストライカー (低い)
    if(who == 1) return l * 5 + 5;  // 1:ガーディアン (高い)
    if(who == 2) return l * 1 + 3;  // 2:クリッパー (低い)
    if(who == 3) return l * 3 + 5;  // 3:メンダー (並)
    if(who == 4) return l * 3 + 5;  // 4:ジャマー (並)
    // 1段階目進化
    if(who == 5) return l * 1 + 5;  // 5:ギア・ソルジャー (低い+)
    if(who == 6) return l * 6 + 8;  // 6:フォートレス (高い+)
    if(who == 7) return l * 1 + 4;  // 7:ブレード・ダンサー (低い+)
    if(who == 8) return l * 3 + 7;  // 8:ハーモニスト (並+)
    if(who == 9) return l * 3 + 6;  // 9:ディスアレンジャー (並)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 6;  // 10:ギア・ナイト (低い+)
    if(who == 11) return l * 7 + 12; // 11:キャッスル・ウォール (高い++)
    if(who == 12) return l * 1 + 5;  // 12:サイクロン・ダンサー (低い+)
    if(who == 13) return l * 4 + 9;  // 13:コンダクター (並++)
    if(who == 14) return l * 4 + 8;  // 14:カオス・ジャマー (並+)
    // 3段階目 (15-21)
    if(who == 15) return l * 3 + 8;  // 15:ヴァンガード (重装甲)
    if(who == 16) return l * 1 + 6;  // 16:スワッシュバックラー (軽装)
    if(who == 17) return l * 9 + 18; // 17:バスティオン (純DEF)
    if(who == 18) return l * 8 + 15; // 18:イージス (DEF+HP)
    if(who == 19) return l * 1 + 6;  // 19:テンペスト
    if(who == 20) return l * 5 + 11; // 20:マエストロ
    if(who == 21) return l * 5 + 10; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 6 + 10;  // 22:オーバーロード
    if(who == 23) return l * 1 + 5;   // 23:デストロイヤー
    if(who == 24) return l * 2 + 7;   // 24:ゼロ・ブレード
    if(who == 25) return l * 1 + 6;   // 25:ヴォイド・ストーカー
    if(who == 26) return l * 12 + 25; // 26:アダマンタイト
    if(who == 27) return l * 10 + 20; // 27:インヴィンシブル
    if(who == 28) return l * 10 + 18; // 28:ディヴァイン・シールド
    if(who == 29) return l * 8 + 15;  // 29:リジェネレーター
    if(who == 30) return l * 3 + 8;   // 30:シルフィード
    if(who == 31) return l * 2 + 7;   // 31:アクセラレーター
    if(who == 32) return l * 6 + 12;  // 32:アーク・ハーモニスト
    if(who == 33) return l * 7 + 14;  // 33:シンフォニア
    if(who == 34) return l * 6 + 12;  // 34:パンデモニウム
    if(who == 35) return l * 5 + 10;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 500 + 200; // 36:はぐれノイズ (超防御)
    if(who == 37) return l * 700 + 300; // 37:ノイズ・キング (極防御)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 3;  // 0:ストライカー (低い)
    if(who == 40) return l * 5 + 5;  // 1:ガーディアン (高い)
    if(who == 41) return l * 1 + 3;  // 2:クリッパー (低い)
    if(who == 42) return l * 3 + 5;  // 3:メンダー (並)
    if(who == 43) return l * 3 + 5;  // 4:ジャマー (並)
    // 1段階目進化
    if(who == 44) return l * 1 + 5;  // 5:ギア・ソルジャー (低い+)
    if(who == 45) return l * 6 + 8;  // 6:フォートレス (高い+)
    if(who == 46) return l * 1 + 4;  // 7:ブレード・ダンサー (低い+)
    if(who == 47) return l * 3 + 7;  // 8:ハーモニスト (並+)
    if(who == 48) return l * 3 + 6;  // 9:ディスアレンジャー (並)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 6;  // 10:ギア・ナイト (低い+)
    if(who == 50) return l * 7 + 12; // 11:キャッスル・ウォール (高い++)
    if(who == 51) return l * 1 + 5;  // 12:サイクロン・ダンサー (低い+)
    if(who == 52) return l * 4 + 9;  // 13:コンダクター (並++)
    if(who == 53) return l * 4 + 8;  // 14:カオス・ジャマー (並+)
    // 3段階目 (15-21)
    if(who == 54) return l * 3 + 8;  // 15:ヴァンガード (重装甲)
    if(who == 55) return l * 1 + 6;  // 16:スワッシュバックラー (軽装)
    if(who == 56) return l * 9 + 18; // 17:バスティオン (純DEF)
    if(who == 57) return l * 8 + 15; // 18:イージス (DEF+HP)
    if(who == 58) return l * 1 + 6;  // 19:テンペスト
    if(who == 59) return l * 5 + 11; // 20:マエストロ
    if(who == 60) return l * 5 + 10; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 6 + 10;  // 22:オーバーロード
    if(who == 62) return l * 1 + 5;   // 23:デストロイヤー
    if(who == 63) return l * 2 + 7;   // 24:ゼロ・ブレード
    if(who == 64) return l * 1 + 6;   // 25:ヴォイド・ストーカー
    if(who == 65) return l * 12 + 25; // 26:アダマンタイト
    if(who == 66) return l * 10 + 20; // 27:インヴィンシブル
    if(who == 67) return l * 10 + 18; // 28:ディヴァイン・シールド
    if(who == 68) return l * 8 + 15;  // 29:リジェネレーター
    if(who == 69) return l * 3 + 8;   // 30:シルフィード
    if(who == 70) return l * 2 + 7;   // 31:アクセラレーター
    if(who == 71) return l * 6 + 12;  // 32:アーク・ハーモニスト
    if(who == 72) return l * 7 + 14;  // 33:シンフォニア
    if(who == 73) return l * 6 + 12;  // 34:パンデモニウム
    if(who == 74) return l * 5 + 10;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 500 + 200; // 36:はぐれノイズ (超防御)
    if(who == 76) return l * 700 + 300; // 37:ノイズ・キング (極防御)

    return 0;
  }

int decide_Speed(int l, int who){
    if(who == 0) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 1) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 2) return l * 5 + 5;  // 2:クリッパー (高い)
    if(who == 3) return l * 2 + 5;  // 3:メンダー (低い)
    if(who == 4) return l * 4 + 3;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 3 + 5;  // 5:ギア・ソルジャー (並)
    if(who == 6) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 7) return l * 6 + 8;  // 7:ブレード・ダンサー (高い+)
    if(who == 8) return l * 2 + 5;  // 8:ハーモニスト (低い)
    if(who == 9) return l * 5 + 4;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 3 + 6;  // 10:ギア・ナイト (並)
    if(who == 11) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 12) return l * 7 + 12; // 12:サイクロン・ダンサー (高い++)
    if(who == 13) return l * 2 + 6;  // 13:コンダクター (低い)
    if(who == 14) return l * 6 + 6;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 2 + 7;  // 15:ヴァンガード (遅い)
    if(who == 16) return l * 7 + 10; // 16:スワッシュバックラー (速い)
    if(who == 17) return l * 1 + 2;  // 17:バスティオン
    if(who == 18) return l * 2 + 3;  // 18:イージス
    if(who == 19) return l * 9 + 15; // 19:テンペスト (最速)
    if(who == 20) return l * 3 + 7;  // 20:マエストロ
    if(who == 21) return l * 7 + 8;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 3 + 8;   // 22:オーバーロード
    if(who == 23) return l * 2 + 7;   // 23:デストロイヤー
    if(who == 24) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 27) return l * 1 + 2;   // 27:インヴィンシブル
    if(who == 28) return l * 3 + 5;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 2 + 4;   // 29:リジェネレーター
    if(who == 30) return l * 11 + 18; // 30:シルフィード
    if(who == 31) return l * 10 + 16; // 31:アクセラレーター
    if(who == 32) return l * 4 + 8;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 6 + 10;  // 33:シンフォニア
    if(who == 34) return l * 8 + 10;  // 34:パンデモニウム
    if(who == 35) return l * 9 + 12;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 10 + 50; // 36:はぐれノイズ (超素早さ)
    if(who == 37) return l * 12 + 60; // 37:ノイズ・キング (極素早さ)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 40) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 41) return l * 5 + 5;  // 2:クリッパー (高い)
    if(who == 42) return l * 2 + 5;  // 3:メンダー (低い)
    if(who == 43) return l * 4 + 3;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 3 + 5;  // 5:ギア・ソルジャー (並)
    if(who == 45) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 46) return l * 6 + 8;  // 7:ブレード・ダンサー (高い+)
    if(who == 47) return l * 2 + 5;  // 8:ハーモニスト (低い)
    if(who == 48) return l * 5 + 4;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 3 + 6;  // 10:ギア・ナイト (並)
    if(who == 50) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 51) return l * 7 + 12; // 12:サイクロン・ダンサー (高い++)
    if(who == 52) return l * 2 + 6;  // 13:コンダクター (低い)
    if(who == 53) return l * 6 + 6;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 2 + 7;  // 15:ヴァンガード (遅い)
    if(who == 55) return l * 7 + 10; // 16:スワッシュバックラー (速い)
    if(who == 56) return l * 1 + 2;  // 17:バスティオン
    if(who == 57) return l * 2 + 3;  // 18:イージス
    if(who == 58) return l * 9 + 15; // 19:テンペスト (最速)
    if(who == 59) return l * 3 + 7;  // 20:マエストロ
    if(who == 60) return l * 7 + 8;  // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 3 + 8;   // 22:オーバーロード
    if(who == 62) return l * 2 + 7;   // 23:デストロイヤー
    if(who == 63) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 66) return l * 1 + 2;   // 27:インヴィンシブル
    if(who == 67) return l * 3 + 5;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 2 + 4;   // 29:リジェネレーター
    if(who == 69) return l * 11 + 18; // 30:シルフィード
    if(who == 70) return l * 10 + 16; // 31:アクセラレーター
    if(who == 71) return l * 4 + 8;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 6 + 10;  // 33:シンフォニア
    if(who == 73) return l * 8 + 10;  // 34:パンデモニウム
    if(who == 74) return l * 9 + 12;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 100 + 500; // 36:はぐれノイズ (超素早さ)
    if(who == 76) return l * 120 + 600; // 37:ノイズ・キング (極素早さ)
    return 0;
  }

int decide_Sense(int l, int who){
    if(who == 0) return l * 1 + 2;  // 0:ストライカー (最低)
    if(who == 1) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 2) return l * 2 + 3;  // 2:クリッパー (低い)
    if(who == 3) return l * 5 + 5;  // 3:メンダー (高い)
    if(who == 4) return l * 4 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 1 + 2;  // 5:ギア・ソルジャー (最低)
    if(who == 6) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 7) return l * 2 + 3;  // 7:ブレード・ダンサー (低い)
    if(who == 8) return l * 6 + 8;  // 8:ハーモニスト (高い+)
    if(who == 9) return l * 5 + 6;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 1 + 2;  // 10:ギア・ナイト (最低)
    if(who == 11) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 12) return l * 2 + 3;  // 12:サイクロン・ダンサー (低い)
    if(who == 13) return l * 7 + 12; // 13:コンダクター (高い++)
    if(who == 14) return l * 6 + 8;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 1 + 2;  // 15:ヴァンガード
    if(who == 16) return l * 3 + 5;  // 16:スワッシュバックラー
    if(who == 17) return l * 1 + 2;  // 17:バスティオン
    if(who == 18) return l * 5 + 10; // 18:イージス (支援)
    if(who == 19) return l * 2 + 3;  // 19:テンペスト
    if(who == 20) return l * 9 + 18; // 20:マエストロ (最高)
    if(who == 21) return l * 8 + 12; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 22) return l * 2 + 3;   // 22:オーバーロード
    if(who == 23) return l * 1 + 2;   // 23:デストロイヤー
    if(who == 24) return l * 4 + 6;   // 24:ゼロ・ブレード
    if(who == 25) return l * 3 + 5;   // 25:ヴォイド・ストーカー
    if(who == 26) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 5;   // 27:インヴィンシブル
    if(who == 28) return l * 8 + 15;  // 28:ディヴァイン・シールド
    if(who == 29) return l * 9 + 18;  // 29:リジェネレーター
    if(who == 30) return l * 3 + 5;   // 30:シルフィード
    if(who == 31) return l * 2 + 4;   // 31:アクセラレーター
    if(who == 32) return l * 12 + 25; // 32:アーク・ハーモニスト
    if(who == 33) return l * 10 + 20; // 33:シンフォニア
    if(who == 34) return l * 11 + 20; // 34:パンデモニウム
    if(who == 35) return l * 9 + 15;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 8 + 25; // 36:はぐれノイズ (旋律なし)
    if(who == 37) return l * 12 + 25; // 37:ノイズ・キング (旋律なし)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 1 + 2;  // 0:ストライカー (最低)
    if(who == 40) return l * 1 + 2;  // 1:ガーディアン (最低)
    if(who == 41) return l * 2 + 3;  // 2:クリッパー (低い)
    if(who == 42) return l * 5 + 5;  // 3:メンダー (高い)
    if(who == 43) return l * 4 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 1 + 2;  // 5:ギア・ソルジャー (最低)
    if(who == 45) return l * 1 + 2;  // 6:フォートレス (最低)
    if(who == 46) return l * 2 + 3;  // 7:ブレード・ダンサー (低い)
    if(who == 47) return l * 6 + 8;  // 8:ハーモニスト (高い+)
    if(who == 48) return l * 5 + 6;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 1 + 2;  // 10:ギア・ナイト (最低)
    if(who == 50) return l * 1 + 2;  // 11:キャッスル・ウォール (最低)
    if(who == 51) return l * 2 + 3;  // 12:サイクロン・ダンサー (低い)
    if(who == 52) return l * 7 + 12; // 13:コンダクター (高い++)
    if(who == 53) return l * 6 + 8;  // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 1 + 2;  // 15:ヴァンガード
    if(who == 55) return l * 3 + 5;  // 16:スワッシュバックラー
    if(who == 56) return l * 1 + 2;  // 17:バスティオン
    if(who == 57) return l * 5 + 10; // 18:イージス (支援)
    if(who == 58) return l * 2 + 3;  // 19:テンペスト
    if(who == 59) return l * 9 + 18; // 20:マエストロ (最高)
    if(who == 60) return l * 8 + 12; // 21:ディスコード
    // 4段階目 (22-35)
    if(who == 61) return l * 2 + 3;   // 22:オーバーロード
    if(who == 62) return l * 1 + 2;   // 23:デストロイヤー
    if(who == 63) return l * 4 + 6;   // 24:ゼロ・ブレード
    if(who == 64) return l * 3 + 5;   // 25:ヴォイド・ストーカー
    if(who == 65) return l * 1 + 2;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 5;   // 27:インヴィンシブル
    if(who == 67) return l * 8 + 15;  // 28:ディヴァイン・シールド
    if(who == 68) return l * 9 + 18;  // 29:リジェネレーター
    if(who == 69) return l * 3 + 5;   // 30:シルフィード
    if(who == 70) return l * 2 + 4;   // 31:アクセラレーター
    if(who == 71) return l * 12 + 25; // 32:アーク・ハーモニスト
    if(who == 72) return l * 10 + 20; // 33:シンフォニア
    if(who == 73) return l * 11 + 20; // 34:パンデモニウム
    if(who == 74) return l * 9 + 15;  // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 8 + 25; // 36:はぐれノイズ (旋律なし)
    if(who == 76) return l * 12 + 25; // 37:ノイズ・キング (旋律なし)
    return 0;
  }

int decide_Luck(int l, int who){
    if(who == 0) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 1) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 2) return l * 4 + 5;  // 2:クリッパー (高い)
    if(who == 3) return l * 3 + 3;  // 3:メンダー (並)
    if(who == 4) return l * 5 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 5) return l * 3 + 3;  // 5:ギア・ソルジャー (並)
    if(who == 6) return l * 2 + 3;  // 6:フォートレス (低い)
    if(who == 7) return l * 5 + 6;  // 7:ブレード・ダンサー (高い+)
    if(who == 8) return l * 3 + 3;  // 8:ハーモニスト (並)
    if(who == 9) return l * 6 + 8;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 10) return l * 3 + 3;  // 10:ギア・ナイト (並)
    if(who == 11) return l * 2 + 3;  // 11:キャッスル・ウォール (低い)
    if(who == 12) return l * 6 + 8;  // 12:サイクロン・ダンサー (高い++)
    if(who == 13) return l * 3 + 3;  // 13:コンダクター (並)
    if(who == 14) return l * 7 + 12; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 15) return l * 4 + 4;  // 15:ヴァンガード
    if(who == 16) return l * 7 + 10; // 16:スワッシュバックラー (クリティカル)
    if(who == 17) return l * 2 + 3;  // 17:バスティオン
    if(who == 18) return l * 4 + 5;  // 18:イージス
    if(who == 19) return l * 7 + 10; // 19:テンペスト
    if(who == 20) return l * 4 + 4;  // 20:マエストロ
    if(who == 21) return l * 9 + 15; // 21:ディスコード (最高)
    // 4段階目 (22-35)
    if(who == 22) return l * 5 + 5;   // 22:オーバーロード
    if(who == 23) return l * 4 + 4;   // 23:デストロイヤー
    if(who == 24) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 25) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 26) return l * 2 + 3;   // 26:アダマンタイト
    if(who == 27) return l * 3 + 4;   // 27:インヴィンシブル
    if(who == 28) return l * 5 + 6;   // 28:ディヴァイン・シールド
    if(who == 29) return l * 4 + 5;   // 29:リジェネレーター
    if(who == 30) return l * 10 + 12; // 30:シルフィード
    if(who == 31) return l * 8 + 10;  // 31:アクセラレーター
    if(who == 32) return l * 5 + 5;   // 32:アーク・ハーモニスト
    if(who == 33) return l * 6 + 6;   // 33:シンフォニア
    if(who == 34) return l * 12 + 20; // 34:パンデモニウム
    if(who == 35) return l * 11 + 18; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 36) return l * 15 + 50; // 36:はぐれノイズ (高幸運)
    if(who == 37) return l * 20 + 60; // 37:ノイズ・キング (高幸運)

    //主人公
    if(who == 38) return l * 3 + 20;
    //仲間
    if(who == 39) return l * 3 + 3;  // 0:ストライカー (並)
    if(who == 40) return l * 2 + 3;  // 1:ガーディアン (低い)
    if(who == 41) return l * 4 + 5;  // 2:クリッパー (高い)
    if(who == 42) return l * 3 + 3;  // 3:メンダー (並)
    if(who == 43) return l * 5 + 5;  // 4:ジャマー (高い)
    // 1段階目進化
    if(who == 44) return l * 3 + 3;  // 5:ギア・ソルジャー (並)
    if(who == 45) return l * 2 + 3;  // 6:フォートレス (低い)
    if(who == 46) return l * 5 + 6;  // 7:ブレード・ダンサー (高い+)
    if(who == 47) return l * 3 + 3;  // 8:ハーモニスト (並)
    if(who == 48) return l * 6 + 8;  // 9:ディスアレンジャー (高い+)
    // 2段階目進化 (10-14)
    if(who == 49) return l * 3 + 3;  // 10:ギア・ナイト (並)
    if(who == 50) return l * 2 + 3;  // 11:キャッスル・ウォール (低い)
    if(who == 51) return l * 6 + 8;  // 12:サイクロン・ダンサー (高い++)
    if(who == 52) return l * 3 + 3;  // 13:コンダクター (並)
    if(who == 53) return l * 7 + 12; // 14:カオス・ジャマー (高い++)
    // 3段階目 (15-21)
    if(who == 54) return l * 4 + 4;  // 15:ヴァンガード
    if(who == 55) return l * 7 + 10; // 16:スワッシュバックラー (クリティカル)
    if(who == 56) return l * 2 + 3;  // 17:バスティオン
    if(who == 57) return l * 4 + 5;  // 18:イージス
    if(who == 58) return l * 7 + 10; // 19:テンペスト
    if(who == 59) return l * 4 + 4;  // 20:マエストロ
    if(who == 60) return l * 9 + 15; // 21:ディスコード (最高)
    // 4段階目 (22-35)
    if(who == 61) return l * 5 + 5;   // 22:オーバーロード
    if(who == 62) return l * 4 + 4;   // 23:デストロイヤー
    if(who == 63) return l * 9 + 12;  // 24:ゼロ・ブレード
    if(who == 64) return l * 10 + 15; // 25:ヴォイド・ストーカー
    if(who == 65) return l * 2 + 3;   // 26:アダマンタイト
    if(who == 66) return l * 3 + 4;   // 27:インヴィンシブル
    if(who == 67) return l * 5 + 6;   // 28:ディヴァイン・シールド
    if(who == 68) return l * 4 + 5;   // 29:リジェネレーター
    if(who == 69) return l * 10 + 12; // 30:シルフィード
    if(who == 70) return l * 8 + 10;  // 31:アクセラレーター
    if(who == 71) return l * 5 + 5;   // 32:アーク・ハーモニスト
    if(who == 72) return l * 6 + 6;   // 33:シンフォニア
    if(who == 73) return l * 12 + 20; // 34:パンデモニウム
    if(who == 74) return l * 11 + 18; // 35:ジョーカー
    // 特殊 (36-37)
    if(who == 75) return l * 15 + 50; // 36:はぐれノイズ (高幸運)
    if(who == 76) return l * 20 + 60; // 37:ノイズ・キング (高幸運)
    return 0;
  }

int normalizeWho(int w) {
    if (w >= 0 && w <= 37) return w + 39;
    return w; 
  }

} // namespace

CoreStatsData CharacterCatalog::statsFor(int characterId, int level) {
  return {
    decide_MaxHP(level, characterId),
    decide_MaxTP(level, characterId),
    decide_Attack(level, characterId),
    decide_Defense(level, characterId),
    decide_Speed(level, characterId),
    decide_Sense(level, characterId),
    decide_Luck(level, characterId)
  };
}

CoreStatsData CharacterCatalog::initialStatsFor(int characterId, int level) {
  switch (characterId) {
    case 80: return {500, 100, 40, 20, 15, 10, 10}; // HP, TP, ATK, DEF, SPD, SEN, LCK
    case 81: return {1200, 200, 70, 60, 10, 20, 20};
    case 82: return {3000, 500, 120, 80, 60, 50, 40};
    case 83: return {8000, 999, 200, 150, 80, 100, 50};
    case 84: return {20000, 9999, 300, 200, 120, 150, 100};
    default: return statsFor(characterId, level);
  }
}

const char* CharacterCatalog::nameFor(int who) {
  const char* name;
    int wd = normalizeWho(who);
    switch (wd) {
      case 38: name = "調律師"; break;
      case 39: name = "ストライカー"; break;
      case 40: name = "ガーディアン"; break;
      case 41: name = "クリッパー"; break;
      case 42: name = "メンダー"; break;
      case 43: name = "ジャマー"; break;
      // 1段階目
      case 44: name = "ギア・ソルジャー"; break;
      case 45: name = "フォートレス"; break;
      case 46: name = "ブレード・ダンサー"; break;
      case 47: name = "ハーモニスト"; break;
      case 48: name = "ディスアレンジャー"; break;
      // 2段階目
      case 49: name = "ギア・ナイト"; break;
      case 50: name = "キャッスル・ウォール"; break;
      case 51: name = "サイクロン・ダンサー"; break;
      case 52: name = "コンダクター"; break;
      case 53: name = "カオス・ジャマー"; break;
      // 3段階目
      case 54: name = "ヴァンガード"; break;
      case 55: name = "スワッシュバックラー"; break;
      case 56: name = "バスティオン"; break;
      case 57: name = "イージス"; break;
      case 58: name = "テンペスト"; break;
      case 59: name = "マエストロ"; break;
      case 60: name = "ディスコード"; break;
      // 4段階目
      case 61: name = "オーバーロード"; break;
      case 62: name = "デストロイヤー"; break;
      case 63: name = "ゼロ・ブレード"; break;
      case 64: name = "ヴォイド・ストーカー"; break;
      case 65: name = "アダマンタイト"; break;
      case 66: name = "インヴィンシブル"; break;
      case 67: name = "ディヴァイン・シールド"; break;
      case 68: name = "リジェネレーター"; break;
      case 69: name = "シルフィード"; break;
      case 70: name = "アクセラレーター"; break;
      case 71: name = "アーク・ハーモニスト"; break;
      case 72: name = "シンフォニア"; break;
      case 73: name = "パンデモニウム"; break;
      case 74: name = "ジョーカー"; break;
      // 特殊
      case 75: name = "はぐれノイズ"; break;
      case 76: name = "ノイズ・キング"; break;
      case 80: name = "スプラウト・ギア"; break;
      case 81: name = "ソレイユ"; break;
      case 82: name = "ハーベスト・スティンガー"; break;
      case 83: name = "雪影"; break;
      case 84: name = "デウス・エクス・マキナ"; break;

      default: name = "???"; break;
    }
    return name;
}
