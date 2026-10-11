#pragma once

class CharacterState;
class Skill;

struct DamageResult {
    bool hit;
    bool critical;
    int damage;
};

struct HealResult {
    bool critical;
    int amount;
};

class CombatCalculator {
public:
    static DamageResult calculateSkillDamage(
        const CharacterState& attacker,
        const CharacterState& target,
        const Skill& skill
    );

    static HealResult calculateHealAmount(
        const CharacterState& healer,
        const Skill& skill
    );
};
