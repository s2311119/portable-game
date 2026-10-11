#pragma once

class CharacterState;

class SkillEffectSystem {
public:
    static void applyBuff(int skillId, CharacterState& target);
    static bool applyDebuff(
        int skillId,
        CharacterState& target,
        const CharacterState* user = nullptr
    );

private:
    static bool checkDebuffHit(
        int baseChance,
        const CharacterState& target,
        const CharacterState* user
    );
    // ★ MaxHP増加処理 (ID 62)
    static void boostMaxHp(CharacterState& target);
};
