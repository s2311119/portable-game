#pragma once

class CharacterState;
class Skill;

struct SkillAudioCue {
    void* context;
    void (*playSE)(void* context, int seId);

    void play(int seId) const { playSE(context, seId); }
};

struct SkillEffectResult {
    int lastDamageDelta = 0;
    const char* actionMessage = nullptr;
};

class SkillEffectSystem {
public:
    static SkillEffectResult execute(
        Skill& skill,
        CharacterState* user,
        CharacterState* target,
        SkillAudioCue audio
    );

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
