#include "ItemCatalog.h"

#include "Skill.h"

Item ItemCatalog::get(int id) {
  
  // --- 0. 素材・換金アイテム (0-99) ---
  
  // 基本素材
  if (id == 1) return Item(1, "鉄くず", Item::ITEM_MATERIAL, 0, 10);
  if (id == 2) return Item(2, "銅の歯車", Item::ITEM_MATERIAL, 0, 30);
  
  // 進化アイテム (値段は適当に設定、売れるようにする)
  if (id == 10) return Item(10, "攻撃の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 11) return Item(11, "赤の歯車",   Item::ITEM_MATERIAL, 0, 100);
  if (id == 12) return Item(12, "紅の歯車",   Item::ITEM_MATERIAL, 0, 500);
  if (id == 13) return Item(13, "防御の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 14) return Item(14, "青の歯車",   Item::ITEM_MATERIAL, 0, 100);
  if (id == 15) return Item(15, "蒼の歯車",   Item::ITEM_MATERIAL, 0, 500);
  if (id == 16) return Item(16, "速度の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 17) return Item(17, "回復の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 18) return Item(18, "感応の歯車", Item::ITEM_MATERIAL, 0, 100);

  // (追加: 金の歯車など)
  if (id == 1010) return Item(1010, "銀の歯車", Item::ITEM_MATERIAL, 0, 200); // ID重複避けるなら変える
  // ※ 元コードにあった id=10,11 は上の進化アイテムと被っているので、
  // 銀の歯車などを別のIDにするか、あるいは上記のID構成に合わせる必要があります。
  // 今回は「進化アイテム」を優先し、銀・金は以下のようにします
  if (id == 3) return Item(3, "銀の歯車", Item::ITEM_MATERIAL, 0, 100);
  if (id == 4) return Item(4, "金の歯車", Item::ITEM_MATERIAL, 0, 500);
  if (id == 5) return Item(5, "謎のパーツ", Item::ITEM_MATERIAL, 0, 1000);


  // --- 1. 消費アイテム (100-199) ---
  if (id == 100) return Item(100, "リペアキット", Item::ITEM_CONSUMABLE, 50, 50);      // 50G
  if (id == 101) return Item(101, "リペアキット中", Item::ITEM_CONSUMABLE, 200, 150);  // 150G
  if (id == 102) return Item(102, "リペアキット大", Item::ITEM_CONSUMABLE, 9999, 500); // 500G
  if (id == 103) return Item(103, "エネルギー缶", Item::ITEM_CONSUMABLE, 20, 200);    // TP回復
  if (id == 104) return Item(104, "万能オイル", Item::ITEM_CONSUMABLE, 0, 100);       // 状態異常回復

  // --- 2. 巻物 (200-299) ---
  if (id >= 200 && id < 300) {
    int skillId = id - 200;
    static Skill tempSkill(skillId); 
    tempSkill = Skill(skillId);
    return Item(id, tempSkill.name, Item::ITEM_CONSUMABLE, 0, 500); 
  }

  // --- 3. 武器 (300-399) ---
  if (id == 300) return Item(300, "練習のタクト", Item::ITEM_WEAPON, 5, 50);
  if (id == 301) return Item(301, "鉄のタクト", Item::ITEM_WEAPON, 15, 300);
  if (id == 302) return Item(302, "強化タクト", Item::ITEM_WEAPON, 30, 1000);
  if (id == 303) return Item(303, "白銀のタクト", Item::ITEM_WEAPON, 50, 3500);
  if (id == 304) return Item(304, "近衛兵のバトン", Item::ITEM_WEAPON, 80, 9000);
  
  // ★ 宝箱限定 (最強)
  if (id == 399) return Item(399, "創世のタクト", Item::ITEM_WEAPON, 1500, 20000);

  // --- 4. 防具 (400-499) ---
  if (id == 400) return Item(400, "旅人のコート", Item::ITEM_ARMOR, 3, 30);
  if (id == 401) return Item(401, "レザーコート", Item::ITEM_ARMOR, 10, 250);
  if (id == 402) return Item(402, "強化スーツ", Item::ITEM_ARMOR, 20, 800);
  if (id == 403) return Item(403, "バトルスーツ", Item::ITEM_ARMOR, 35, 3000);
  if (id == 404) return Item(404, "指揮官の軍服", Item::ITEM_ARMOR, 60, 8000);

  // ★ 宝箱限定 (最強)
  if (id == 499) return Item(499, "イージス・ギア", Item::ITEM_ARMOR, 1200, 20000);

  // --- 5. アクセサリー (500-599) ---
  if (id == 500) return Item(500, "パワーリング", Item::ITEM_ACCESSORY, 10, 400); // ATK
  if (id == 501) return Item(501, "ガードリング", Item::ITEM_ACCESSORY, 10, 400); // DEF
  if (id == 502) return Item(502, "クイックブーツ", Item::ITEM_ACCESSORY, 10, 400); // SPD
  if (id == 503) return Item(503, "スナイパーレンズ", Item::ITEM_ACCESSORY, 10, 400); // SEN
  if (id == 504) return Item(504, "ラッキーコイン", Item::ITEM_ACCESSORY, 50, 400); // LCK
  
  // ★ 宝箱限定 (最強)
  if (id == 599) return Item(599, "トリニティ・コア", Item::ITEM_ACCESSORY, 500, 15000);

  // デフォルト
  return Item(id, "不明な品", Item::ITEM_MATERIAL, 0, 1);
}
