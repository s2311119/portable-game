#pragma once

#include <Arduino.h>
#include <optional>
#include <vector>

#include "CharacterState.h"
#include "Enemy.h"
#include "Player.h"
#include "Party.h"
#include "Graphic.h"
#include "Controller.h"
#include "MUSIC.h"
#include "Sd.h"
#include "Inventory.h"
#include "Vibration.h"
#include "Map.h"
#include "WorldProgress.h"
#include "ImageManager.h"
#include "LegacyGameTypes.h"
#include "Skill.h"
#include "EncounterSpec.h"
#include "BattleTypes.h"

class Battle {
private:
  // Battle クラス内 (private 推奨)

  // Battle クラス内 (private)

  CharacterState* resolveTarget(CharacterState* originalTarget, Skill& skill);

  bool executeHostileSkillOnTarget(
      Skill& skill,
      CharacterState* attacker,
      CharacterState* originalTarget,
      MUSIC& music,
      bool allowTargetRedirect);

public:
  enum BattleState{
    STATE_TURN_START,     // 0. ターン開始
    STATE_ACTOR_SELECT,     // 1. 行動者を決定
    STATE_COMMAND_SELECT,   // 2. コマンド入力
    STATE_SKILL_SELECT,     // 3. 旋律入力
    STATE_TARGET_SELECT_ENEMY, // 4. ターゲット選択 (敵)
    STATE_TARGET_SELECT_ALLY,  // 5. ターゲット選択 (味方)
    
    // 6. 行動前・継続効果処理
    STATE_PRE_ACTION_EFFECTS, 
    
    STATE_ENEMY_AI,         // 7. 敵のAI実行
    STATE_ACTION_READY,     // 8. 行動内容をBattleActionとして確定
    STATE_EXECUTE_NEXT_ACTION, // 9. 
    STATE_ACTION_CALC,      // 10. 行動実行（計算）
    STATE_ACTION_MSG,       // 11. 行動結果のメッセージ
    STATE_CHECK_BATTLE_RESULT, // 12. 勝敗だけ確認
    STATE_END_ACTOR_TURN,      // 13. 現在の行動者のターン終了処理
    STATE_SHOW_XP_GAIN,     // 14. XP獲得表示
    STATE_BATTLE_END,       // 15. 戦闘終了
    STATE_CHECK_STATUS,      // 16. Yボタンステータス
    STATE_SHOW_RESULT_MESSAGE // 17. メッセージ表示
  };
  BattleState currentBattleState;
  static const int CMD_COUNT = 4;
  const char* commands[CMD_COUNT] = {"たたかう", "にげる", "せんりつ", "ぼうぎょ"};

  // UIカーソルは用途ごとに分離
  int commandSelected = 0;
  int skillSelected = 0;

  enum class BattleEndReason {
    NONE,
    VICTORY,
    DEFEAT,
    ESCAPE
  };

  BattleEndReason battleEndReason = BattleEndReason::NONE;

  enum class BattlePhase {
    PLANNING,   // 行動を決めている段階
    EXECUTING   // 決めた行動を実行している段階
  };

  BattlePhase battlePhase = BattlePhase::PLANNING;

  bool inBattle = false;
  bool heroLeveledUp = false;
  bool isBossBattleMode = false;

  enum class ActionType {
    NONE,
    ATTACK,
    SKILL,
    DEFEND,
    FLEE
  };

  enum class ActionPriority {
    NORMAL = 0,
    FIRST_STRIKE = 1
  };

  struct BattleAction {
    std::optional<CombatantRef> actor;          // 行動するキャラ
    ActionType type = ActionType::NONE;
    int skillIndex = -1;             // SKILLの場合のみ使用
    std::optional<CombatantRef> target;         // 単体対象。全体技などでは対象なし可

    ActionPriority priority = ActionPriority::NORMAL;
    int speedSnapshot = 0;           // 行動決定時点のSPD
  };

  enum class MessageFlow {
    ACTION_COMPLETE, // 実際の行動が終わった
    CONTINUE_TURN,   // 毒・回復等の表示後、まだ行動する
    END_TURN         // 死亡・麻痺等でこの人の行動を終了
  };

  ActionType pendingActionType = ActionType::NONE;
  int pendingSkillIndex = -1;

  MessageFlow messageFlow = MessageFlow::ACTION_COMPLETE;

  int enemyTargetIndex = 0;   // 敵リスト(activeEnemies)のカーソル (targetIndexから変更)
  int allyTargetIndex = 0;    // 味方リストのカーソル (新規)
  std::optional<CombatantRef> currentTarget; // 現在の対象

  String message = "";
  std::vector<Enemy> activeEnemies;
  Player* heroRef = nullptr; // 戦闘中の主人公への参照
  Party* partyRef = nullptr;   // 戦闘中の仲間パーティーへの参照

  //bool shouldReturncs = false;
  //bool iscaclated = false;
  //int lastGainedXP = 0;

  // ★★★ 行動順管理リスト ★★★
  std::vector<CombatantRef> actorList; // 戦闘参加者 (Speed順)
  int currentActorIndex = 0;        // 現在行動中のキャラのインデックス
  std::optional<CombatantRef> currentActor; // 現在行動中のキャラ
  // 1ターン分の行動予定
  std::vector<BattleAction> plannedActions;
  BattleAction currentBattleAction;
  int currentPlannedActionIndex = 0;
  int lastDamage = 0;             // 直前のダメージ計算結果
  String lastActionMessage = "";
  int lastGainedXP = 0;
  String lastEvolvedName = ""; // ★★★ これを追加 ★★★
  int turnCount = 1;
  int statusPage = 0;
  //std::vector<uint16_t*> enemyImages; // 敵の画像 (64x64)
  //std::vector<uint16_t*> allyImages;  // 味方の画像 (64x64)
  uint16_t* activeImages[8];
  std::vector<String> resultMessages;
  int resultMessageIndex = 0;

  BattleAction createBattleAction(
      CharacterState* actor,
      ActionType type,
      int skillIndex = -1,
      CharacterState* target = nullptr);

  Battle();

  void start(MUSIC &music, Sd &sd, Player& hero, Party& party, const EncounterSpec& spec, ImageManager &imgMgr);
  std::optional<BattleResult> takeResult();

  void update(Controller &ctrl, GameState &state, MUSIC &music, Sd &sd, Player &hero, Inventory &inventory, Vibration &vibration, Map &map, WorldProgress& progress, Graphic &g);
  
  // ★ 引数を Graphic* に変更
  void draw(Graphic* g);
  ~Battle() = default;
private:
  BattleKind currentBattleKind = BattleKind::NORMAL;
  std::optional<BattleResult> pendingResult;

  std::optional<CombatantRef> refFor(CharacterState* combatant) const;
  CharacterState* resolveCombatant(const std::optional<CombatantRef>& ref);
  
  void ExecuteSkill(Skill& skill, CharacterState* user, CharacterState* target, MUSIC& music);

  // 生きている味方のリストを取得する
  std::vector<CharacterState*> getAliveAllies();

  std::vector<CharacterState*> getDeadAllies();

  std::vector<CharacterState*> getAllAllies();

  std::vector<CharacterState*> getAliveEnemies();
};
