#pragma once

class EvolutionCatalog {
public:
  enum EvolutionTriggerType {
    TRIGGER_LEVEL, // レベルアップ
    TRIGGER_ITEM   // アイテム使用
  };

  struct EvolutionRule {
    int before_who_id;
    EvolutionTriggerType trigger_type;
    int trigger_value; 
    int after_who_id;
  };

  static constexpr int RULE_COUNT = 32;
  static constexpr int LINEAGE_CAPACITY = RULE_COUNT + 1;

  struct RuleView {
    const EvolutionRule* rules;
    int count;
  };

  struct Lineage {
    int ids[LINEAGE_CAPACITY];
    int count;
  };

  static RuleView getRules();
  static Lineage getLineage(int characterId);
};
