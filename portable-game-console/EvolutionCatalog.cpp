#include "EvolutionCatalog.h"

namespace {

const EvolutionCatalog::EvolutionRule evolutionRules[] = {
  // --- 仲間1号機 (Attack: 39) の進化ツリー ---
  { 39, EvolutionCatalog::TRIGGER_LEVEL, 10, 44 }, // 39(Lv10) -> 44
  { 44, EvolutionCatalog::TRIGGER_LEVEL, 20, 49 }, // 44(Lv20) -> 49
  { 49, EvolutionCatalog::TRIGGER_LEVEL, 30, 54 }, // 49(Lv30) -> 54
  { 49, EvolutionCatalog::TRIGGER_ITEM,  10, 55 }, // 49(Lv40) -> 55 (Aルート)
  { 54, EvolutionCatalog::TRIGGER_LEVEL, 50, 61 }, // 54 + アイテム10 -> 61 (Bルート)
  { 54, EvolutionCatalog::TRIGGER_ITEM,  11, 62 }, // 54 + アイテム11 -> 62
  { 55, EvolutionCatalog::TRIGGER_LEVEL, 50, 63 }, // 55(Lv40) -> 63 (Aルート)
  { 55, EvolutionCatalog::TRIGGER_ITEM,  12, 64 }, // 55 + アイテム12 -> 64 (Bルート)

  // --- 仲間2号機 (Defense: 48) の進化ツリー ---
  { 40, EvolutionCatalog::TRIGGER_LEVEL, 10, 45 }, // 48(Lv10) -> 49
  { 45, EvolutionCatalog::TRIGGER_LEVEL, 20, 50 }, // 49(Lv20) -> 50
  { 50, EvolutionCatalog::TRIGGER_LEVEL, 30, 56 }, // 50(Lv30) -> 51
  { 50, EvolutionCatalog::TRIGGER_ITEM,  13, 57 }, // 51(Lv40) -> 52 (Aルート)
  { 56, EvolutionCatalog::TRIGGER_LEVEL, 50, 65 }, // 51 + アイテム13 -> 53 (Bルート)
  { 56, EvolutionCatalog::TRIGGER_ITEM,  14, 66 }, // 50 + アイテム14 -> 54
  { 57, EvolutionCatalog::TRIGGER_LEVEL, 50, 67 }, // 54(Lv40) -> 55 (Aルート)
  { 57, EvolutionCatalog::TRIGGER_ITEM,  15, 68 }, // 54 + アイテム15 -> 56 (Bルート)

  // --- 仲間3号機 (Speed: 57) の進化ツリー ---
  { 41, EvolutionCatalog::TRIGGER_LEVEL, 10, 46 },
  { 46, EvolutionCatalog::TRIGGER_LEVEL, 20, 51 },
  { 51, EvolutionCatalog::TRIGGER_LEVEL, 30, 58 },
  { 58, EvolutionCatalog::TRIGGER_LEVEL, 50, 69 }, // 60(Lv40) -> 61 (Aルート)
  { 58, EvolutionCatalog::TRIGGER_ITEM,  16, 70 }, // 60 + アイテム16 -> 62 (Bルート)

  // --- 仲間4号機 (Heal: 63) の進化ツリー ---
  { 42, EvolutionCatalog::TRIGGER_LEVEL, 10, 47 },
  { 47, EvolutionCatalog::TRIGGER_LEVEL, 20, 52 },
  { 52, EvolutionCatalog::TRIGGER_LEVEL, 30, 59 },
  { 59, EvolutionCatalog::TRIGGER_LEVEL, 50, 71 }, // 66(Lv40) -> 67 (Aルート)
  { 59, EvolutionCatalog::TRIGGER_ITEM,  17, 72 }, // 66 + アイテム17 -> 68 (Bルート)

  // --- 仲間5号機 (Sense: 69) の進化ツリー ---
  { 43, EvolutionCatalog::TRIGGER_LEVEL, 10, 48 },
  { 48, EvolutionCatalog::TRIGGER_LEVEL, 20, 53 },
  { 53, EvolutionCatalog::TRIGGER_LEVEL, 30, 60 },
  { 60, EvolutionCatalog::TRIGGER_LEVEL, 50, 73 }, // 72(Lv40) -> 73 (Aルート)
  { 60, EvolutionCatalog::TRIGGER_ITEM,  18, 74 }, // 72 + アイテム18 -> 74 (Bルート)

  //特殊
  { 75, EvolutionCatalog::TRIGGER_ITEM,   4, 76 } // 金の歯車で進化
};

static_assert(sizeof(evolutionRules) / sizeof(evolutionRules[0]) == EvolutionCatalog::RULE_COUNT,
              "Evolution rule count must match the lineage capacity");

int previousFormOf(int currentId) {
    for (int i = 0; i < EvolutionCatalog::RULE_COUNT; i++) {
      // 進化テーブルの「進化後(after)」が自分と一致したら、「進化前(before)」を返す
      if (evolutionRules[i].after_who_id == currentId) {
        return evolutionRules[i].before_who_id;
      }
    }
    return -1; // 進化前なし
  }

} // namespace

EvolutionCatalog::RuleView EvolutionCatalog::getRules() {
    return {evolutionRules, RULE_COUNT};
}

EvolutionCatalog::Lineage EvolutionCatalog::getLineage(int characterId) {
    Lineage lineage = {};
    int currentId = characterId;

    while (currentId != -1) {
        if (lineage.count >= LINEAGE_CAPACITY) break;

        bool repeated = false;
        for (int i = 0; i < lineage.count; ++i) {
            if (lineage.ids[i] == currentId) {
                repeated = true;
                break;
            }
        }
        if (repeated) break;

        lineage.ids[lineage.count] = currentId;
        ++lineage.count;
        currentId = previousFormOf(currentId);
    }
    return lineage;
}
