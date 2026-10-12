#include "Battle.h"

#include "CharacterCatalog.h"
#include "CharacterProgressionSystem.h"
#include "ItemCatalog.h"
#include "SkillCatalog.h"
#include "SkillEffectSystem.h"

#include <algorithm>
#include <utility>


CharacterState* Battle::resolveTarget(CharacterState* originalTarget, Skill& skill) {
    if (originalTarget == nullptr) return nullptr; // 安全対策
    if (originalTarget->stats().getHp() <= 0) return originalTarget;
    
    // 自分・全体・味方への補助技は対象変更しない。
    // RANDOM_ENEMY は各ヒットごとに resolveTarget() を通す。
    if (skill.scope == Skill::TargetScope::SELF ||
        skill.scope == Skill::TargetScope::ALL_ENEMIES ||
        skill.scope == Skill::TargetScope::ALL_ALLIES ||
        skill.type == Skill::EffectType::HEAL ||
        skill.type == Skill::EffectType::REVIVE ||
        skill.type == Skill::EffectType::BUFF)
    {
      return originalTarget;
    }
    // --- 1. 挑発 (Taunt) ---
    CharacterState* tauntTarget = nullptr;
    if (originalTarget->isPlayer()) { // 敵→味方
         if (originalTarget->effects().getTauntTurns() > 0) return originalTarget;
         if (heroRef && heroRef->state().effects().getTauntTurns() > 0 && heroRef->state().stats().getHp() > 0) tauntTarget = &heroRef->state();
         if (partyRef) {
             for (size_t i = 0; i < partyRef->memberCount(); ++i) {
                 CharacterState& ally = partyRef->memberState(i);
                 if (ally.effects().getTauntTurns() > 0 && ally.stats().getHp() > 0) tauntTarget = &ally;
             }
         }
    } else { // 味方→敵
         if (originalTarget->effects().getTauntTurns() > 0) return originalTarget;
         std::vector<CharacterState*> taunters;
         for (auto& enemy : activeEnemies) {
             if (enemy.state().effects().getTauntTurns() > 0 && enemy.state().stats().getHp() > 0) {
                 taunters.push_back(&enemy.state());
             }
         }
         if (!taunters.empty()) tauntTarget = taunters[random(0, taunters.size())];
    }
    if (tauntTarget != nullptr) return tauntTarget;

    // --- 2. ステルス (Untargetable) ---
    // 狙われた人がステルス中なら、別の生存者に逸らす
    if (originalTarget->effects().getUntargetableTurns() > 0) {
         std::vector<CharacterState*> others;
         if (originalTarget->isPlayer()) {
             // 主人公・仲間から「自分以外」かつ「生存者」を探す
             if (&heroRef->state() != originalTarget && heroRef->state().stats().getHp() > 0) others.push_back(&heroRef->state());
             for (size_t i = 0; i < partyRef->memberCount(); ++i) {
                 CharacterState& member = partyRef->memberState(i);
                 if (&member != originalTarget && member.stats().getHp() > 0) others.push_back(&member);
             }
         } else {
             // 敵から「自分以外」かつ「生存者」を探す
             for (auto& e : activeEnemies) {
                 if (&e.state() != originalTarget && e.state().stats().getHp() > 0) others.push_back(&e.state());
             }
         }
         
         // ターゲット変更 (他に誰もいなければそのまま攻撃を受ける)
         if (!others.empty()) {
             return others[random(0, others.size())];
         }
    }

    // --- 3. かばう (Cover) ---
    // 味方限定
    if (originalTarget->isPlayer()) {
         // 誰かが「かばう」発動中か？
         if (&heroRef->state() != originalTarget && heroRef->state().effects().getCoverTurns() > 0 && heroRef->state().stats().getHp() > 0) return &heroRef->state();
         for (size_t i = 0; i < partyRef->memberCount(); ++i) {
             CharacterState& ally = partyRef->memberState(i);
             if (&ally != originalTarget && ally.effects().getCoverTurns() > 0 && ally.stats().getHp() > 0) return &ally;
         }
    }

    return originalTarget;
  }

bool Battle::executeHostileSkillOnTarget(
      Skill& skill,
      CharacterState* attacker,
      CharacterState* originalTarget,
      AudioManager& music,
      bool allowTargetRedirect) {
    if (attacker == nullptr ||
        originalTarget == nullptr ||
        originalTarget->stats().getHp() <= 0)
    {
      return false;
    }

    // 単体攻撃・ランダム攻撃では
    // 挑発・ステルス・かばうによる対象変更を行う。
    // 全体攻撃では行わない。
    CharacterState* defender = originalTarget;

    if (allowTargetRedirect) {
      defender = resolveTarget(originalTarget, skill);
    }

    if (defender == nullptr ||
        defender->stats().getHp() <= 0)
    {
      return false;
    }

    // 反射は「攻撃」にだけ適用する。
    // 純粋なデバフなど OTHER カテゴリは反射しない。
    bool isOffensive =
        skill.category == Skill::Category::PHYSICAL ||
        skill.category == Skill::Category::MELODY;

    bool isReflected = false;

    if (isOffensive) {
      // キングス・シールド：
      // 有効ターン中は物理・旋律を何度でも反射する
      if (defender->effects().getReflectAllTurns() > 0) {
        isReflected = true;
      }

      // リフレクト：
      // 物理攻撃を1回だけ反射して、その場で消費する
      else if (defender->effects().getReflectPhysicalTurns() > 0 &&
              skill.category == Skill::Category::PHYSICAL)
      {
        isReflected = true;
        defender->consumePhysicalReflection();
      }

      // 旋律反射：
      // 有効ターン中は旋律攻撃を反射する
      // 反射した回数ではなくUpdateTurn()で寿命を減らす
      else if (defender->effects().getReflectMagicTurns() > 0 &&
              skill.category == Skill::Category::MELODY)
      {
        isReflected = true;
      }
    }

    if (isReflected) {
      music.playSE(7);

      message +=
          "\n" +
          String(defender->name()) +
          " は跳ね返した！";

      // 反射された場合は使用者自身へ実行
      ExecuteSkill(
          skill,
          attacker,
          attacker,
          music
      );

      return false;
    }

    // カウンター判定のため、攻撃前HPを覚えておく
    int hpBefore = defender->stats().getHp();

    ExecuteSkill(
        skill,
        attacker,
        defender,
        music
    );

    bool tookDamage =
        defender->stats().getHp() < hpBefore;

    // 実際に物理ダメージを受けたときだけ反撃
    if (tookDamage &&
        defender->stats().getHp() > 0 &&
        defender->effects().getCounterTurns() > 0 &&
        skill.category == Skill::Category::PHYSICAL &&
        defender != attacker)
    {
      message +=
          "\n" +
          String(defender->name()) +
          " の反撃！";

      music.playSE(3);

      int counterDamage =
          defender->attack();

      counterDamage =
          attacker->takeDamage(
              counterDamage,
              Skill::Category::PHYSICAL
          );

      attacker->addPopup(
          counterDamage,
          false
      );
    }

    return defender->stats().getHp() <= 0;
  }

std::optional<CombatantRef> Battle::refFor(CharacterState* combatant) const {
    if (combatant == nullptr) return std::nullopt;

    if (combatant == &heroRef->state()) {
      return CombatantRef{CombatantSide::PLAYER, 0};
    }
    for (size_t i = 0; i < partyRef->memberCount(); ++i) {
      if (combatant == &partyRef->memberState(i)) {
        return CombatantRef{CombatantSide::ALLY, static_cast<int>(i)};
      }
    }
    for (size_t i = 0; i < activeEnemies.size(); ++i) {
      if (combatant == &activeEnemies[i].state()) {
        return CombatantRef{CombatantSide::ENEMY, static_cast<int>(i)};
      }
    }
    return std::nullopt;
  }

CharacterState* Battle::resolveCombatant(
    const std::optional<CombatantRef>& ref) {
    if (!ref) return nullptr;

    switch (ref->side) {
      case CombatantSide::PLAYER:
        return &heroRef->state();
      case CombatantSide::ALLY:
        return &partyRef->memberState(ref->index);
      case CombatantSide::ENEMY:
        return &activeEnemies[ref->index].state();
    }
    return nullptr;
  }

Battle::BattleAction Battle::createBattleAction(
      CharacterState* actor,
      ActionType type,
      int skillIndex,
      CharacterState* target) {
    BattleAction action;

    action.actor = refFor(actor);
    action.type = type;
    action.skillIndex = skillIndex;
    action.target = refFor(target);

    if (actor != nullptr) {
      action.speedSnapshot = actor->speed();
    }

    // 先制技判定
    if (type == ActionType::SKILL && actor != nullptr) {
      const SkillSet& skills = actor->skills();

      if (skillIndex >= 0 &&
          skillIndex < static_cast<int>(skills.count()))
      {
        int skillId = skills.skillIdAt(skillIndex);

        // 必ず先制
        if (skillId == 38 ||   // ソニックブレイド
            skillId == 84 ||   // マッハブレイド
            skillId == 121)    // ソニックダガー
        {
          action.priority = ActionPriority::FIRST_STRIKE;
        }
      }
    }

    return action;
  }

Battle::Battle() {
    activeEnemies.reserve(4);
    plannedActions.reserve(8);
  }

void Battle::beginStart(Player& hero, Party& party, const EncounterSpec& spec) {
    if (inBattle) return;
    pendingResult.reset();
    currentBattleKind = spec.kind;
    inBattle = true;
    message = "敵が現れた！";
    isBossBattleMode = false; 
    battleEndReason = BattleEndReason::NONE;
    commandSelected = 0;
    skillSelected = 0;
    turnCount = 1;
    heroLeveledUp = false;
    // ★★★ 変数の完全リセット (重要) ★★★
    pendingActionType = ActionType::NONE;
    pendingSkillIndex = -1;
    messageFlow = MessageFlow::ACTION_COMPLETE;
    currentTarget = std::nullopt;
    currentActor = std::nullopt;
    currentActorIndex = 0;
    lastDamage = 0;
    lastActionMessage = "";
    enemyTargetIndex = 0;
    allyTargetIndex = 0;
    plannedActions.clear();
    currentBattleAction = BattleAction();
    battlePhase = BattlePhase::PLANNING;
    currentPlannedActionIndex = 0;

    // 参加メンバーへの参照を保存
    heroRef = &hero;
    partyRef = &party;
    activeEnemies.clear();
    activeEnemies.reserve(4);
    if (spec.kind != BattleKind::NORMAL) {
      isBossBattleMode = true;
      for (const EnemySpawnSpec& spawn : spec.enemies) {
        activeEnemies.push_back(Enemy::fromRolledLevel(spawn.enemyId, spawn.level));
      }
      Enemy& boss = activeEnemies.back();
      boss.name = boss.state().name();
    }else{
      for (const EnemySpawnSpec& spawn : spec.enemies) {
        activeEnemies.push_back(Enemy::fromRolledLevel(spawn.enemyId, spawn.level));
        Enemy& newEnemy = activeEnemies.back();

        newEnemy.state().clearBattleEffects();
        newEnemy.name = newEnemy.state().name();
      }

    }
  }

void Battle::finishStart() {
    Party& party = *partyRef;
    // ★ 行動順リストを作成 (ソートはしない)
    actorList.clear();
    actorList.push_back({CombatantSide::PLAYER, 0});
    for (size_t i = 0; i < party.memberCount(); ++i) {
      actorList.push_back({CombatantSide::ALLY, static_cast<int>(i)});
    }
    // ★ 敵全員を行動順リストに追加
    for (size_t i = 0; i < activeEnemies.size(); ++i) {
      actorList.push_back({CombatantSide::ENEMY, static_cast<int>(i)});
    }
    
    // ★ ターン開始処理へ
    currentBattleState = STATE_TURN_START;
  }

void Battle::update(Controller &ctrl, AudioManager &music, Player &hero, Inventory &inventory, Vibration &vibration, const BattleBgmHooks& bgm) {
    if (!inBattle) return;
    CharacterState* currentActor = resolveCombatant(this->currentActor);
    CharacterState* currentTarget = resolveCombatant(this->currentTarget);
    CharacterState* originalActor = currentActor;
    CharacterState* originalTarget = currentTarget;
    Party& party = *partyRef; 

    bool aPressed = ctrl.pressedDebounced(ctrl.BTN_A);
    bool bPressed = ctrl.pressedDebounced(ctrl.BTN_B);
    bool yPressed = ctrl.pressedDebounced(ctrl.BTN_Y);
    bool upPressed = ctrl.pressedDebounced(ctrl.BTN_UP);
    bool downPressed = ctrl.pressedDebounced(ctrl.BTN_DOWN);
    bool rightPressed = ctrl.pressedDebounced(ctrl.BTN_RIGHT);
    bool leftPressed = ctrl.pressedDebounced(ctrl.BTN_LEFT);

    // --- Yボタン（ステータス確認）の最優先処理 ---
    if (yPressed) {
      if (currentBattleState == STATE_COMMAND_SELECT) {
        music.playSE(2);
        currentBattleState = STATE_CHECK_STATUS;
        statusPage = 0; // ★ 最初は主人公(0)から
      }
    } else if (currentBattleState == STATE_CHECK_STATUS) {
      // ★ 左右でページ切り替え
      int totalMembers = 1 + party.memberCount(); // 主人公 + 仲間数
      
      if (leftPressed) {
          statusPage = (statusPage - 1 + totalMembers) % totalMembers;
      }
      if (rightPressed) {
          statusPage = (statusPage + 1) % totalMembers;
      }

      if (aPressed || bPressed || yPressed) {
        music.playSE(1);
        currentBattleState = STATE_COMMAND_SELECT;
      }
      return; 
    }

    // --- メインの戦闘ステートマシン ---
    switch(currentBattleState){

      // 0. ターン開始処理
      case STATE_TURN_START:
        message = "ターン " + String(turnCount);

        if (aPressed) {

          // -------------------------
          // 新しいターンの行動予約を開始
          // -------------------------
          battlePhase = BattlePhase::PLANNING;

          plannedActions.clear();
          currentPlannedActionIndex = 0;

          currentActorIndex = 0;
          currentActor = nullptr;
          currentBattleAction = BattleAction();

          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;

          currentBattleState = STATE_ACTOR_SELECT;
        }
        break;

      // 1. 行動者を決定する
      case STATE_ACTOR_SELECT: {
        // -------------------------
        // 全員分の行動予約が完了した
        // -------------------------
        if (currentActorIndex >= actorList.size()) {

          // 先制技を最優先、その中ではSPD順
          std::stable_sort(
            plannedActions.begin(),
            plannedActions.end(),
            [](const BattleAction& a, const BattleAction& b) {

              if (a.priority != b.priority) {
                return static_cast<int>(a.priority) >
                      static_cast<int>(b.priority);
              }

              return a.speedSnapshot > b.speedSnapshot;
            }
          );

          battlePhase = BattlePhase::EXECUTING;
          currentPlannedActionIndex = 0;

          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // -------------------------
        // 次に行動を決めるキャラ
        // -------------------------
        currentActor = resolveCombatant(actorList[currentActorIndex]);

        if (currentActor == nullptr) {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // 死亡中なら行動予約しない
        if (currentActor->stats().getHp() <= 0) {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // 前のキャラの行動内容を残さない
        pendingActionType = ActionType::NONE;
        pendingSkillIndex = -1;
        currentTarget = nullptr;

        // ★ここでは状態異常処理をしない
        // 実際に行動する直前に PRE_ACTION_EFFECTS を通す

        if (currentActor->isPlayer()) {
          currentBattleState = STATE_COMMAND_SELECT;
        }
        else {
          currentBattleState = STATE_ENEMY_AI;
        }

        break;
      }

      // 2. 味方のコマンド入力
      case STATE_COMMAND_SELECT: 
        message = String(currentActor->name()) + "：コマンドを選ぼう！";
        if (leftPressed) {commandSelected = (commandSelected - 1 + CMD_COUNT) % CMD_COUNT; music.playSE(2);}
        if (upPressed)   {commandSelected = (commandSelected - 2 + CMD_COUNT) % CMD_COUNT; music.playSE(2);}
        if (rightPressed){commandSelected = (commandSelected + 1) % CMD_COUNT; music.playSE(2);}
        if (downPressed) {commandSelected = (commandSelected + 2) % CMD_COUNT; music.playSE(2);}

        if (aPressed) {
          if (commandSelected == 0) { // たたかう
            music.playSE(0);
            pendingActionType = ActionType::ATTACK;
            pendingSkillIndex = -1;
            enemyTargetIndex = 0;   // ★ 敵カーソルをリセット
            currentBattleState = STATE_TARGET_SELECT_ENEMY; // ★ 敵選択へ
          } 
          else if (commandSelected == 1) { // にげる
            music.playSE(0);
            message = "逃げ出した！";
            battleEndReason = BattleEndReason::ESCAPE;
            currentBattleState = STATE_BATTLE_END;
          } 
          else if (commandSelected == 2) { // せんりつ
            if (currentActor->effects().getSilencedTurns() > 0) {
              music.playSE(1);
              message = String(currentActor->name()) + " は沈黙している！";
              // (Aボタン押しっぱなしループを防ぐ)
              if(ctrl.pressed(ctrl.BTN_A)) delay(200);
            } else {
              music.playSE(0);
              skillSelected = 0;
              currentBattleState = STATE_SKILL_SELECT;
            }
          }
          else if (commandSelected == 3) {
            music.playSE(0);
            pendingActionType = ActionType::DEFEND;
            pendingSkillIndex = -1;
            
            // ターゲット選択は不要なので、行動確定へ
            currentTarget = currentActor;
            currentBattleState = STATE_ACTION_READY;
          }
        }
        break;

      
      // 3. 旋律（せんりつ）選択
      case STATE_SKILL_SELECT: {
        int skillCount = currentActor->skills().count();
        if (skillCount == 0) {
          message = "覚えている旋律がない！";
          if(aPressed) currentBattleState = STATE_COMMAND_SELECT;
          break;
        }
        
        message = "どの旋律を使う？";
        if (upPressed) {
          skillSelected = (skillSelected - 1 + skillCount) % skillCount;
          music.playSE(2);
        }
        if (downPressed) {
          skillSelected = (skillSelected + 1) % skillCount;
          music.playSE(2);
        }

        if (aPressed) {
          Skill skill = SkillCatalog::get(currentActor->skills().skillIdAt(skillSelected));
          
          if (currentActor->stats().getTp() >= skill.tpCost) {
            music.playSE(0);

            pendingActionType = ActionType::SKILL;
            pendingSkillIndex = skillSelected;
            
            // ★★★ スキルの対象範囲(scope)によって分岐 ★★★
            switch (skill.scope) {
              case Skill::TargetScope::SELF:
                currentTarget = currentActor;
                currentBattleState = STATE_ACTION_READY;
                break;
              case Skill::TargetScope::SINGLE_ENEMY:
                enemyTargetIndex = 0; // 敵カーソルをリセット
                currentBattleState = STATE_TARGET_SELECT_ENEMY;
                break;
              case Skill::TargetScope::SINGLE_ALLY:
                allyTargetIndex = 0; // 味方カーソルをリセット
                currentBattleState = STATE_TARGET_SELECT_ALLY;
                break;
              case Skill::TargetScope::ALL_ENEMIES:
              case Skill::TargetScope::ALL_ALLIES:
              case Skill::TargetScope::RANDOM_ENEMY:
                currentTarget = nullptr;
                currentBattleState = STATE_ACTION_READY;
                break;
            }
          } else {
            music.playSE(1);
            message = "TPが足りない！";
            // (Aボタン押しっぱなしでTP不足ループするのを防ぐ)
            if(ctrl.pressed(ctrl.BTN_A)) delay(200);
            currentBattleState = STATE_COMMAND_SELECT;
          }
        }
        // Bボタンでコマンド選択に戻る
        if (bPressed) {
          music.playSE(1);
          currentBattleState = STATE_COMMAND_SELECT;
        }
        break;
      }

      // ★★★ 4. ターゲット選択 (敵) ★★★
      case STATE_TARGET_SELECT_ENEMY: {
        message = "誰を狙う？";
        while (activeEnemies[enemyTargetIndex].state().stats().getHp() <= 0){
          enemyTargetIndex = (enemyTargetIndex - 1 + activeEnemies.size()) % activeEnemies.size();
        }
        // 生きている敵だけを選べるように
        if (leftPressed) {
          do {
            enemyTargetIndex = (enemyTargetIndex - 1 + activeEnemies.size()) % activeEnemies.size();
          } while (activeEnemies[enemyTargetIndex].state().stats().getHp() <= 0);
          music.playSE(2);
        }
        if (rightPressed) {
          do {
            enemyTargetIndex = (enemyTargetIndex + 1) % activeEnemies.size();
          } while (activeEnemies[enemyTargetIndex].state().stats().getHp() <= 0);
          music.playSE(2);
        }

        // Aボタンで決定
        if (aPressed) {
          music.playSE(0);
          currentTarget = &activeEnemies[enemyTargetIndex].state();
          currentBattleState = STATE_ACTION_READY;
        }
        // Bボタンでキャンセル
        if (bPressed) {
          music.playSE(1);
          if (pendingActionType == ActionType::SKILL) {
            currentBattleState = STATE_SKILL_SELECT;
          } else {
            currentBattleState = STATE_COMMAND_SELECT;
          }
        }
        break;
      }
      
      // 5. ターゲット選択 (味方)
      case STATE_TARGET_SELECT_ALLY: {
        message = "誰にかける？";

        bool isReviveSkill = false;

        if (pendingActionType == ActionType::SKILL) {
          const SkillSet& skills = currentActor->skills();

          if (pendingSkillIndex >= 0 &&
              pendingSkillIndex < static_cast<int>(skills.count())) {
            isReviveSkill =
                (SkillCatalog::get(skills.skillIdAt(pendingSkillIndex)).type ==
                Skill::EffectType::REVIVE);
          }
        }

        std::vector<CharacterState*> allies =
            isReviveSkill ? getDeadAllies()
                          : getAliveAllies();

        if (allies.empty()) {
          if (isReviveSkill) {
            message = "倒れている味方がいない！";
          }

          currentBattleState = STATE_SKILL_SELECT;
          break;
        }

        if (upPressed) {
          allyTargetIndex = (allyTargetIndex - 1 + allies.size()) % allies.size();
          music.playSE(2);
        }
        if (downPressed) {
          allyTargetIndex = (allyTargetIndex + 1) % allies.size();
          music.playSE(2);
        }

        if (aPressed) {
          music.playSE(0);
          currentTarget = allies[allyTargetIndex];
          currentBattleState = STATE_ACTION_READY;
        }
        // Bボタンで旋律選択に戻る
        if (bPressed) {
          music.playSE(1);
          currentBattleState = STATE_SKILL_SELECT;
        }
        break;
      }

      // 6. 行動前・継続効果処理
      case STATE_PRE_ACTION_EFFECTS: {
        message = "";
        lastDamage = 0;
        
        // 1. ターン消費と、毒・リジェネの効果を適用
        lastDamage = currentActor->ApplyTurnEffects();

        if (lastDamage > 0) { // 毒ダメージ
          music.playSE(3);
          message = String(currentActor->name()) + " は毒:" + String(lastDamage) + " のダメージ！";
        } else if (lastDamage < 0) { // HPリジェネ
          music.playSE(7);
          message = String(currentActor->name()) + " は回復:" + String(-lastDamage) + " 回復！";
        }
        if (currentActor->stats().getHp() <= 0) {
          // オートリバイブ判定
          if (currentActor->effects().getAutoReviveTurns() > 0) {
            currentActor->consumeAutoRevive(); // 効果消費
            currentActor->revive(50); // HP半分で復活
            music.playSE(7);
            message += "\n" + String(currentActor->name()) + " は自動蘇生した！";
          }else{
            message = String(currentActor->name()) + " は力尽きた…";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }
        }

        // 3. 強制行動不能チェック
        if (currentActor->effects().getCantMoveTurns() > 0) {
          if (message != "") {
            message += "\n";
          }

          message += String(currentActor->name()) + " は動けない！";

          currentActor->consumeBlockedAction();

          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // 4. 麻痺(Paralyzed)チェック (isParalyzed -> getParalyzedTurns)
        if (currentActor->effects().getParalyzedTurns() > 0) {
          if (random(100) < 50) { // (例: 50%で行動不能)
            message = String(currentActor->name()) + " は痺れて動けない！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }
        }
        
        // 5. 混乱(Confused)チェック
        if (currentActor->effects().getConfusedTurns() > 0) {
          message = String(currentActor->name()) + " は混乱している！";
          // (例: 50%で暴走)
          if (random(100) < 50) {
              message += "\nわけがわからず攻撃！";
              // pendingAction = 0;
              pendingActionType = ActionType::ATTACK;
              pendingSkillIndex = -1;
              
              // ターゲットを敵味方全員からランダムで選択
              std::vector<CharacterState*> allTargets = getAliveAllies(); // 味方リスト
              for (auto& enemy : activeEnemies) { // 敵リスト
                  if(enemy.state().stats().getHp() > 0) allTargets.push_back(&enemy.state());
              }
              
              if (allTargets.empty()) {
                currentTarget = nullptr;
              } else {
                currentTarget = allTargets[random(0, allTargets.size())];
              }

              currentBattleState = STATE_ACTION_READY;
              break;
          }
          // (暴走しなかった場合は、メッセージ表示(Aボタン待ち)へ)
        }

        // 6. メッセージがあるか？
        if (message != "") { // 毒/回復/混乱(不発)のメッセージがあった
          messageFlow = MessageFlow::CONTINUE_TURN;
          currentBattleState = STATE_ACTION_MSG;
        } else {
          // 継続効果が何もなければ、通常の行動へ
          if (battlePhase == BattlePhase::EXECUTING) {
            // すでに行動内容は決まっているので、そのまま実行
            currentBattleState = STATE_ACTION_CALC;
          }
          else {
            // まだ行動を決める段階
            if (currentActor->isPlayer()) {
              currentBattleState = STATE_COMMAND_SELECT;
            } else {
              currentBattleState = STATE_ENEMY_AI;
            }
          }
        }
        break;
      }

      // 7. 敵のAI実行
      case STATE_ENEMY_AI: {
        String enemyName = currentActor->name();
        message = enemyName + " の攻撃!";
        // pendingAction = 0; // デフォルトは通常攻撃
        pendingActionType = ActionType::ATTACK;
        pendingSkillIndex = -1;
        
        bool willFlee = false;
        // はぐれノイズ(36): 50%で逃げる
        int who = currentActor->id();
        if (who == 36 && random(100) < 30) willFlee = true;
        
        // ノイズ・キング(37): 70%で逃げる (より逃げやすい)
        if (who == 37 && random(100) < 40) willFlee = true;
        if (willFlee) {
          pendingActionType = ActionType::FLEE;
          pendingSkillIndex = -1;
          message = enemyName + " は逃げ出した！";
          currentBattleState = STATE_ACTION_READY;
          break;
        }
        // 1. デフォルトターゲット (プレイヤー側からランダム)
        std::vector<CharacterState*> playerSide = getAliveAllies();
        if (playerSide.empty()) currentTarget = nullptr; 
        else currentTarget = playerSide[random(0, playerSide.size())];

        // 2. スキル選択ロジック
        const SkillSet& skills = currentActor->skills();
        
        if (skills.count() != 0 && currentActor->effects().getSilencedTurns() <= 0) {
          int who = currentActor->id();
          int curTP = currentActor->stats().getTp();
          int curHP = currentActor->stats().getHp();
          int maxHP = currentActor->stats().getMaxHp();
          int selectedSkillIndex = -1; 

          // [ヘルパー] IDでスキルを探す
          auto findSkillIndex = [&](int id) -> int {
            for(int i=0; i<skills.count(); i++) {
              if(skills.skillIdAt(i) == id && SkillCatalog::get(skills.skillIdAt(i)).tpCost <= curTP) return i;
            }
            return -1;
          };

          // --- 特殊AI ---
          if (who == 80) { // スプラウト・ギア
            if (curHP < maxHP / 2 && random(100) < 50) selectedSkillIndex = findSkillIndex(47); 
            else if (random(100) < 30) selectedSkillIndex = findSkillIndex(21); 
          }
          else if (who == 81) { // ソレイユ
            if (curHP < maxHP / 2) { 
              if (random(100) < 60) selectedSkillIndex = findSkillIndex(34); 
              else if (random(100) < 40) selectedSkillIndex = findSkillIndex(87); 
            } else { 
              if (random(100) < 50) selectedSkillIndex = findSkillIndex(52); 
            }
          }
          else if (who == 82) { // ハーベスト・スティンガー
            if (currentTarget && currentTarget->effects().getPoisonTurns() > 0) selectedSkillIndex = findSkillIndex(42);
            else if (random(100) < 70) {
              int r = random(3);
              if (r==0) selectedSkillIndex = findSkillIndex(105); 
              else if(r==1) selectedSkillIndex = findSkillIndex(45); 
              else selectedSkillIndex = findSkillIndex(113); 
            }
          }
          else if (who == 83) { // 雪影
            if (curHP < maxHP / 3) selectedSkillIndex = findSkillIndex(123); 
            else if (turnCount % 3 == 0) selectedSkillIndex = findSkillIndex(random(100)<50 ? 39 : 73);
          }
          else if (who == 84) { // ラスボス
            if (curHP < maxHP / 4) selectedSkillIndex = findSkillIndex(127); 
            else if (turnCount % 4 == 0) selectedSkillIndex = findSkillIndex(114); 
            else if (random(100) < 30) selectedSkillIndex = findSkillIndex(86);
          }
          // --- 回復役AI ---
          else if (who == 3 ||
                  who == 8 ||
                  who == 13 ||
                  who == 20 ||
                  who == 32)
          {
            // HPが半分未満の生存中の味方を探す
            CharacterState* injuredTarget = nullptr;

            for (auto& e : activeEnemies) {
              if (e.state().stats().getHp() > 0 &&
                  e.state().stats().getHp() < e.state().stats().getMaxHp() / 2)
              {
                injuredTarget = &e.state();
                break;
              }
            }

            // 回復が必要な味方がいる場合だけ回復技を探す
            if (injuredTarget != nullptr) {

              int healIdx = -1;
              int bestHealPower = -1;

              for (int i = 0;
                  i < static_cast<int>(skills.count());
                  i++)
              {
                Skill skill = SkillCatalog::get(skills.skillIdAt(i));

                // 使用可能な回復スキルだけを対象にする
                if (skill.type == Skill::EffectType::HEAL &&
                    skill.tpCost <= curTP)
                {
                  // 複数ある場合は威力が高いものを使う
                  if (skill.power > bestHealPower) {
                    bestHealPower = skill.power;
                    healIdx = i;
                  }
                }
              }

              if (healIdx != -1) {
                selectedSkillIndex = healIdx;
                currentTarget = injuredTarget;
              }
            }
          }

          // --- 汎用AI ---
          if (selectedSkillIndex == -1 && random(100) < 30) {
            std::vector<int> usableIndices;
            for(int i=0; i<skills.count(); i++) {
              if (SkillCatalog::get(skills.skillIdAt(i)).tpCost <= curTP) usableIndices.push_back(i);
            }
            if (!usableIndices.empty()) {
              selectedSkillIndex = usableIndices[random(0, usableIndices.size())];
            }
          }

          // --- 行動確定後のターゲット調整 ---
          if (selectedSkillIndex != -1) {
            pendingActionType = ActionType::SKILL;
            pendingSkillIndex = selectedSkillIndex;
            Skill s = SkillCatalog::get(skills.skillIdAt(selectedSkillIndex));
            message = enemyName + " の " + s.name + "!";
            
            // ★★★ 重要: スキル範囲によるターゲットの強制変更 ★★★
            
            // 自分のみ
            if (s.scope == Skill::TargetScope::SELF) {
              currentTarget = currentActor;
            }
            // 味方単体（敵から見た味方＝他の敵）
            else if (s.scope == Skill::TargetScope::SINGLE_ALLY) {
              // もしターゲットが「プレイヤー側」になっていたら、「敵側」に切り替える
              if (currentTarget == nullptr || currentTarget->isPlayer()) {
                // 生きている敵リストを作成
                std::vector<CharacterState*> livingEnemies;
                for(auto& e : activeEnemies) if(e.state().stats().getHp() > 0) livingEnemies.push_back(&e.state());
                
                if (!livingEnemies.empty()) {
                  // ランダムな仲間にかける（回復AIですでに決まっている場合は上書きしないよう注意が必要だが、
                  // ここでは「回復AI以外で選ばれた補助魔法」を想定してランダムにする）
                  // ※回復AIでcurrentTargetを変えた場合、isPlayer()はfalseなのでここには入らない。完璧。
                  currentTarget = livingEnemies[random(0, livingEnemies.size())];
                } else {
                  currentTarget = currentActor; // 誰もいなければ自分
                }
              }
            }
            // (全体攻撃やランダム攻撃の場合、STATE_ACTION_CALC でループ処理されるので currentTarget は無視でOK)
          }
        }
        
        currentBattleState = STATE_ACTION_READY;
        break;
      }

      // 8. 
      case STATE_ACTION_READY: {
        currentBattleAction = createBattleAction(
            currentActor,
            pendingActionType,
            pendingSkillIndex,
            currentTarget
        );

        // -------------------------
        // 行動予約フェーズ
        // -------------------------
        if (battlePhase == BattlePhase::PLANNING) {

          plannedActions.push_back(currentBattleAction);

          // 次のキャラへ
          currentActorIndex++;

          // 前の行動情報をクリア
          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;
          currentBattleAction = BattleAction();

          currentBattleState = STATE_ACTOR_SELECT;
          break;
        }

        // -------------------------
        // 実行フェーズ
        // -------------------------
        // 混乱などによって実行直前に行動内容が変更された場合は
        // 新たに予約せず、そのまま実行する。
        currentBattleState = STATE_ACTION_CALC;

        break;
      }

      // 9. 
      case STATE_EXECUTE_NEXT_ACTION: {
        // 全ての予約行動を実行し終えた
        if (currentPlannedActionIndex >= plannedActions.size()) {
          // 今のターンの全行動が終了したので、
          // 次のターン番号へ進める
          turnCount++;

          battlePhase = BattlePhase::PLANNING;
          currentBattleState = STATE_TURN_START;
          break;
        }

        currentBattleAction =
            plannedActions[currentPlannedActionIndex];

        currentActor = resolveCombatant(currentBattleAction.actor);

        // nullptrならスキップ
        if (currentActor == nullptr) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // ★このキャラの実際の行動順が来たので、
        // 前回の「ぼうぎょ」をここで解除する
        currentActor->endDefense();

        // 行動前に倒されていたキャラはスキップ
        if (currentActor->stats().getHp() <= 0) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
          break;
        }

        // 予約していた行動を既存実行系へ戻す
        pendingActionType = currentBattleAction.type;
        pendingSkillIndex = currentBattleAction.skillIndex;
        currentTarget = resolveCombatant(currentBattleAction.target);

        // 毒・麻痺・混乱などの状態異常を処理
        currentBattleState = STATE_PRE_ACTION_EFFECTS;

        break;
      }

      // 10. 行動の実行（計算）
      case STATE_ACTION_CALC: {
        lastDamage = 0;
        lastActionMessage = "";

        messageFlow = MessageFlow::ACTION_COMPLETE;
        // ポップアップを安全にクリア
        if (heroRef) heroRef->state().clearPopups();
        if (partyRef) {
            for(size_t i = 0; i < partyRef->memberCount(); ++i) partyRef->memberState(i).clearPopups();
        }
        for(auto& e : activeEnemies) e.state().clearPopups();

        if (pendingActionType == ActionType::FLEE) {
          // 1. 経験値とドロップを0にする (倒した扱いにはしない)
          // (StatusからはEnemyオブジェクトに辿れないので、リストから探す)
          for (auto& e : activeEnemies) {
            if (&e.state() == currentActor) {
              e.xpYield = 0;
              e.dropItemId = 0;
              break;
            }
          }

          // 2. HPを0にして強制退場させる
          // (ダメージ処理ではなく、直接0を代入)
          currentActor->retireFromBattle(); 

          // 3. 効果音 (逃走音)
          music.playSE(3); // シュッという音など

          // 4. メッセージ表示へ
          messageFlow = MessageFlow::ACTION_COMPLETE;
          currentBattleState = STATE_ACTION_MSG;
          break; 
        }
        if (pendingActionType == ActionType::DEFEND) { // 防御
          // 1. 防御フラグを立てる
          currentActor->beginDefense();

          // 2. 音とメッセージ
          music.playSE(5); // 何か適当な音（物理防御音など）
          message = String(currentActor->name()) + " は身を守っている！";
          
          // 3. メッセージ表示ステートへ
          messageFlow = MessageFlow::ACTION_COMPLETE;
          currentBattleState = STATE_ACTION_MSG;
          break; 
        }
        // 行動者がいない場合はスキップ
        if (!currentActor) {
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // スキル情報取得
        Skill currentSkill = SkillCatalog::get(0);

        // 実行時に予約対象を変更したか
        bool targetChanged = false;

        // 行動予約後に沈黙を受けている可能性があるため、
        // 実行時にもスキル使用可否を確認する
        if (pendingActionType == ActionType::SKILL &&
            currentActor->effects().getSilencedTurns() > 0)
        {
          message =
              String(currentActor->name()) +
              " は沈黙していて旋律を使えない！";

          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        if (pendingActionType == ActionType::ATTACK) {
          currentSkill = SkillCatalog::get(0);
          currentSkill.name = "攻撃";
          currentSkill.power = 50;
          currentSkill.type = Skill::EffectType::DAMAGE;
          currentSkill.scope = Skill::TargetScope::SINGLE_ENEMY;
          currentSkill.dependence = Skill::StatDependence::ATK;
          currentSkill.category = Skill::Category::PHYSICAL;

          // 予約していた攻撃対象がすでに倒れていた場合
          if (currentTarget == nullptr ||
              currentTarget->stats().getHp() <= 0)
          {
            std::vector<CharacterState*> candidates =
                currentActor->isPlayer()
                    ? getAliveEnemies()
                    : getAliveAllies();

            if (candidates.empty()) {
              message =
                  String(currentActor->name()) +
                  " の攻撃！\n対象がいない！";

              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          message = String(currentActor->name()) + " の攻撃！";

          if (targetChanged) {
            message += "\n対象を変更した！";
          }
        }
        else if (pendingActionType == ActionType::SKILL) {
          const SkillSet& skills = currentActor->skills();

          if (pendingSkillIndex < 0 ||
            pendingSkillIndex >= skills.count()) {

            message = "スキル指定エラー";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          currentSkill = SkillCatalog::get(skills.skillIdAt(pendingSkillIndex));

          // --- 実行時ターゲット再確認 ---

          // 単体の敵対象技で、予約していた相手がすでに倒れていた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ENEMY &&
              (currentTarget == nullptr || currentTarget->stats().getHp() <= 0))
          {
            // 使用者から見た「敵」の生存者を取得
            std::vector<CharacterState*> candidates =
                currentActor->isPlayer()
                    ? getAliveEnemies()
                    : getAliveAllies();

            if (candidates.empty()) {
              message += "\n対象がいない！";
              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            // 生存している敵からランダムで新しい対象を選ぶ
            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          // 単体蘇生の対象が、実行前に別の蘇生で既に復活していた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ALLY &&
              currentSkill.type == Skill::EffectType::REVIVE &&
              (currentTarget == nullptr || currentTarget->stats().getHp() > 0))
          {
            message += "\n対象はすでに復活している！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          // 単体回復・単体バフの予約対象が倒れていた場合
          if (currentSkill.scope == Skill::TargetScope::SINGLE_ALLY &&
              (currentSkill.type == Skill::EffectType::HEAL ||
              currentSkill.type == Skill::EffectType::BUFF) &&
              (currentTarget == nullptr || currentTarget->stats().getHp() <= 0))
          {
            // 使用者から見た生存中の味方
            std::vector<CharacterState*> candidates =
                currentActor->isPlayer()
                    ? getAliveAllies()
                    : getAliveEnemies();

            if (candidates.empty()) {
              message =
                  String(currentActor->name()) +
                  " の " +
                  currentSkill.name +
                  "！\n対象がいない！";

              messageFlow = MessageFlow::END_TURN;
              currentBattleState = STATE_ACTION_MSG;
              break;
            }

            currentTarget =
                candidates[random((int)candidates.size())];

            targetChanged = true;
          }

          // TP消費をここに集約
          if (!currentActor->spendTP(currentSkill.tpCost)) {
            message = String(currentActor->name()) + " はTPが足りない！";
            messageFlow = MessageFlow::END_TURN;
            currentBattleState = STATE_ACTION_MSG;
            break;
          }

          message =
            String(currentActor->name()) +
            " の " +
            currentSkill.name +
            "！";

          if (targetChanged) {
            message += "\n対象を変更した！";
          }
        }
        else {
          message = "行動指定エラー";
          messageFlow = MessageFlow::END_TURN;
          currentBattleState = STATE_ACTION_MSG;
          break;
        }

        // --- 実行ロジック ---
        // A. 敵単体
        if (currentSkill.scope ==
            Skill::TargetScope::SINGLE_ENEMY)
        {
          if (currentTarget != nullptr &&
              currentTarget->stats().getHp() > 0)
          {
            executeHostileSkillOnTarget(
                currentSkill,
                currentActor,
                currentTarget,
                music,
                true   // 挑発・ステルス・かばうあり
            );
          }
        }

        // B. 味方単体・自分
        else if (
            currentSkill.scope ==
                Skill::TargetScope::SINGLE_ALLY ||
            currentSkill.scope ==
                Skill::TargetScope::SELF)
        {
          bool targetCanReceiveSkill =
              currentTarget != nullptr &&
              (
                currentTarget->stats().getHp() > 0 ||
                currentSkill.type ==
                    Skill::EffectType::REVIVE
              );

          if (targetCanReceiveSkill) {
            ExecuteSkill(
                currentSkill,
                currentActor,
                currentTarget,
                music
            );
          }
        }

        // C. 敵全体
        else if (
            currentSkill.scope ==
                Skill::TargetScope::ALL_ENEMIES)
        {
          bool anyDefeated = false;

          std::vector<CharacterState*> targets =
              currentActor->isPlayer()
                  ? getAliveEnemies()
                  : getAliveAllies();

          for (auto target : targets) {

            bool defeated =
                executeHostileSkillOnTarget(
                    currentSkill,
                    currentActor,
                    target,
                    music,
                    false  // 全体攻撃なので対象変更なし
                );

            if (defeated) {
              anyDefeated = true;
            }
          }

          if (anyDefeated) {
            if (currentActor->isPlayer()) {
              message += "\n敵を倒した！";
            }
            else {
              message += "\n味方が倒れた！";
            }
          }
        }

        // D. 味方全体
        else if (
            currentSkill.scope ==
                Skill::TargetScope::ALL_ALLIES)
        {
          std::vector<CharacterState*> targets;

          if (currentActor->isPlayer()) {

            if (currentSkill.type ==
                Skill::EffectType::REVIVE)
            {
              // 全体蘇生は死亡者も含める
              targets = getAllAllies();
            }
            else {
              targets = getAliveAllies();
            }
          }
          else {
            // 敵側
            targets = getAliveEnemies();
          }

          for (auto target : targets) {
            ExecuteSkill(
                currentSkill,
                currentActor,
                target,
                music
            );
          }
        }

        // E. ランダム敵
        else if (
            currentSkill.scope ==
                Skill::TargetScope::RANDOM_ENEMY)
        {
          int hits = 1;

          if (currentSkill.id == 123) {
            hits = random(2, 5);
          }

          if (currentSkill.id == 127) {
            hits = random(1, 8);
          }

          std::vector<CharacterState*> targets =
              currentActor->isPlayer()
                  ? getAliveEnemies()
                  : getAliveAllies();

          if (!targets.empty()) {

            bool anyDefeated = false;

            for (int i = 0; i < hits; i++) {

              // 前のヒットで倒れている可能性があるので
              // 毎回生存者だけを作り直す
              std::vector<CharacterState*> live;

              for (auto target : targets) {
                if (target != nullptr &&
                    target->stats().getHp() > 0)
                {
                  live.push_back(target);
                }
              }

              if (live.empty()) {
                break;
              }

              CharacterState* target =
                  live[random(0, live.size())];

              bool defeated =
                  executeHostileSkillOnTarget(
                      currentSkill,
                      currentActor,
                      target,
                      music,
                      true   // 1ヒットごとに対象変更あり
                  );

              if (defeated) {
                anyDefeated = true;
              }
            }

            if (anyDefeated) {
              if (currentActor->isPlayer()) {
                message += "\n敵を倒した！";
              }
              else {
                message += "\n味方が倒れた！";
              }
            }
          }
        }

        // 特殊効果メッセージ
        if (lastActionMessage != "") message += "\n" + lastActionMessage;

        // 行動後フラグ処理
        if (currentSkill.id == 126) {
          currentActor->applyActionBlock(1);
        }
        if (currentSkill.id == 85) currentActor->enableDoubleAction(); 

        // 振動処理
        if (currentTarget && !currentTarget->popupValues.empty()) {
            auto& lastPopup = currentTarget->popupValues.back();
            if (currentTarget->isPlayer() && lastPopup.value > 0) vibration.trigger(50);
            else if (!currentTarget->isPlayer() && lastPopup.isCritical) vibration.trigger(150);
            else if (!currentTarget->isPlayer() && lastPopup.value > 0) vibration.trigger(30);
        }

        currentBattleState = STATE_ACTION_MSG;
        break;
      }
      
      // 11. 行動結果のメッセージ表示 (待機のみ)
      case STATE_ACTION_MSG:
        if (aPressed) {
          hero.state().clearPopups();

          for (size_t i = 0; i < party.memberCount(); ++i) {
            party.memberState(i).clearPopups();
          }

          for (auto& e : activeEnemies) {
            e.state().clearPopups();
          }

          if (messageFlow == MessageFlow::CONTINUE_TURN) {
            if (battlePhase == BattlePhase::EXECUTING) {
              // 毒やリジェネのメッセージを見終えたので、
              // 予約済みの行動をそのまま実行する
              currentBattleState = STATE_ACTION_CALC;
            }
            else {
              // まだ行動決定段階
              if (currentActor->isPlayer()) {
                currentBattleState = STATE_COMMAND_SELECT;
              } else {
                currentBattleState = STATE_ENEMY_AI;
              }
            }
          }
          else {
            // ACTION_COMPLETE / END_TURN のどちらでも
            // まず戦闘が終わったか確認する。
            currentBattleState = STATE_CHECK_BATTLE_RESULT;
          }
        }
        break;
      // 12. 勝利判定 (旧 8)
      case STATE_CHECK_BATTLE_RESULT: {
        // -------------------------
        // 1. 敵側が全滅しているか
        // -------------------------
        bool allEnemiesDefeated = true;

        for (auto& enemy : activeEnemies) {
          if (enemy.state().stats().getHp() > 0) {
            allEnemiesDefeated = false;
            break;
          }
        }

        // -------------------------
        // 2. プレイヤー側が全滅しているか
        // -------------------------
        bool anyoneAlive = false;

        if (hero.state().stats().getHp() > 0) {
          anyoneAlive = true;
        }

        if (!anyoneAlive) {
          for (size_t i = 0; i < party.memberCount(); ++i) {
            if (party.memberState(i).stats().getHp() > 0) {
              anyoneAlive = true;
              break;
            }
          }
        }

        // -------------------------
        // 3. 勝敗判定
        // -------------------------

        // 両陣営が同時に全滅した場合。
        // 今回は既存挙動をなるべく維持する。
        if (allEnemiesDefeated && !anyoneAlive) {
          if (currentActor && currentActor->isPlayer()) {
            message = "敵を倒した！";
            battleEndReason = BattleEndReason::VICTORY;

            if (aPressed) {
              currentBattleState = STATE_SHOW_XP_GAIN;
            }
          }
          else {
            message = "全滅した…";
            battleEndReason = BattleEndReason::DEFEAT;

            if (aPressed) {
              currentBattleState = STATE_BATTLE_END;
            }
          }

          break;
        }

        // 敵全滅
        if (allEnemiesDefeated) {
          message = "敵を倒した！";
          battleEndReason = BattleEndReason::VICTORY;

          if (aPressed) {
            currentBattleState = STATE_SHOW_XP_GAIN;
          }

          break;
        }

        // 味方全滅
        if (!anyoneAlive) {
          message = "全滅した…";
          battleEndReason = BattleEndReason::DEFEAT;

          if (aPressed) {
            currentBattleState = STATE_BATTLE_END;
          }

          break;
        }

        // -------------------------
        // 4. まだ戦闘継続
        // -------------------------
        currentBattleState = STATE_END_ACTOR_TURN;
        break;
      }
      // 13. 
      case STATE_END_ACTOR_TURN: {
        // -------------------------
        // 1. 2回行動
        // -------------------------
        // 実際に行動を完了していて、
        // かつ本人がまだ生きている場合のみ再行動する
        if (messageFlow == MessageFlow::ACTION_COMPLETE &&
            currentActor != nullptr &&
            currentActor->stats().getHp() > 0 &&
            currentActor->effects().getIsDoubleAction())
        {
          currentActor->consumeDoubleAction();

          // 前の行動情報をクリア
          pendingActionType = ActionType::NONE;
          pendingSkillIndex = -1;
          currentTarget = nullptr;
          messageFlow = MessageFlow::ACTION_COMPLETE;

          if (currentActor->isPlayer()) {
            message = String(currentActor->name()) + " の再行動！";
            currentBattleState = STATE_COMMAND_SELECT;
          }
          else {
            currentBattleState = STATE_ENEMY_AI;
          }

          break;
        }

        // -------------------------
        // 2. この行動者のターン終了
        // -------------------------
        if (currentActor != nullptr) {
          currentActor->UpdateTurn();
        }

        // 次の行動に古い情報を持ち越さない
        pendingActionType = ActionType::NONE;
        pendingSkillIndex = -1;
        currentTarget = nullptr;
        messageFlow = MessageFlow::ACTION_COMPLETE;

        // -------------------------
        // 3. 次の行動者へ
        // -------------------------
        if (battlePhase == BattlePhase::EXECUTING) {
          currentPlannedActionIndex++;
          currentBattleState = STATE_EXECUTE_NEXT_ACTION;
        }
        else {
          currentActorIndex++;
          currentBattleState = STATE_ACTOR_SELECT;
        }
        break;
      }

      // 14. XP獲得 (旧 10)
      case STATE_SHOW_XP_GAIN:
        {
          // メッセージキューをクリア
          resultMessages.clear();
          resultMessageIndex = 0;

          lastGainedXP = 0;
          for (auto& enemy : activeEnemies) {
            lastGainedXP += enemy.xpYield;
          }
          
          // 1. 獲得メッセージ
          resultMessages.push_back(String(lastGainedXP) + "Expを得た!");
          
          // 2. レベルアップ・進化判定 (主人公)
          if (hero.state().stats().getHp() > 0) {
            const ProgressionResult res = CharacterProgressionSystem::gainExperience(hero.state(), lastGainedXP);
            if (res.newLevel > res.oldLevel) resultMessages.push_back("調律師はLv" + String(hero.state().progress().level()) + "になった!");
          }
          // 3. レベルアップ・進化判定 (仲間)
          for(size_t i = 0; i < party.memberCount(); ++i) {
            CharacterState& ally = party.memberState(i);
            if (ally.stats().getHp() > 0) {
              const ProgressionResult allyRes = CharacterProgressionSystem::gainExperience(ally, lastGainedXP);
              if (allyRes.newLevel > allyRes.oldLevel) resultMessages.push_back(String(ally.name()) + "はLv" + String(ally.progress().level()) + "になった!");
              if (allyRes.evolved) {
                resultMessages.push_back(
                    String(CharacterCatalog::nameFor(allyRes.oldCharacterId)) +
                    "は" +
                    String(CharacterCatalog::nameFor(allyRes.newCharacterId)) +
                    "に進化した!"
                );
              }
            }
          }
          
          // 4. ドロップ判定
          // 敵ごとに設定されている dropItemId / dropRatePercent を使う。
          // 逃走した敵は dropItemId = 0 にされているため対象外。
          for (auto& enemy : activeEnemies) {
            if (enemy.dropItemId <= 0 ||
                enemy.dropRatePercent <= 0)
            {
              continue;
            }

            // 0～99 の乱数がドロップ率未満なら入手
            if (random(0, 100) < enemy.dropRatePercent) {
              Item droppedItem =
                  ItemCatalog::get(enemy.dropItemId);

              inventory.addItem(droppedItem);

              resultMessages.push_back(
                  String(droppedItem.name) +
                  "を手に入れた!"
              );
            }
          }

          // ★ 最初のメッセージを表示して、順次表示ステートへ
          message = resultMessages[0];
          currentBattleState = STATE_SHOW_RESULT_MESSAGE;
        }
        break;
      
      //15. 
      case STATE_SHOW_RESULT_MESSAGE:
        if (aPressed) {
          resultMessageIndex++;
          if (resultMessageIndex < resultMessages.size()) {
            message = resultMessages[resultMessageIndex];
          } else {
            // 全て表示し終わったら終了
            currentBattleState = STATE_BATTLE_END;
          }
        }
        break;
      // 16. 戦闘終了 (旧 13)
      case STATE_BATTLE_END:
        if(bPressed || aPressed){
          if (battleEndReason == BattleEndReason::VICTORY) {
            pendingResult = BattleResult{BattleOutcome::VICTORY, currentBattleKind};
          } else if (battleEndReason == BattleEndReason::DEFEAT) {
            pendingResult = BattleResult{BattleOutcome::DEFEAT, currentBattleKind};
          } else if (battleEndReason == BattleEndReason::ESCAPE) {
            pendingResult = BattleResult{BattleOutcome::ESCAPE, currentBattleKind};
          }
          bool actualBossDefeated =
            isBossBattleMode &&
            battleEndReason == BattleEndReason::VICTORY;
          inBattle = false;
          if (battleEndReason != BattleEndReason::DEFEAT && !actualBossDefeated) {
            bgm.emit(BattleBgmEvent::EARLY_FIELD_RETURN, currentBattleKind);
          }
          
          // 戦闘終了時にステータスをリセット
          hero.state().clearBattleEffects();
          std::vector<CharacterState::PopupData>().swap(hero.state().popupValues);
          for(size_t i = 0; i < party.memberCount(); ++i) {
            CharacterState& member = party.memberState(i);
            member.clearBattleEffects();
            std::vector<CharacterState::PopupData>().swap(member.popupValues);
          }
          activeEnemies.clear(); // リストを空にする
          // ★ ベクタの容量自体を解放するテクニック (Swap idiom)
          std::vector<Enemy>().swap(activeEnemies);

          // 3. メッセージリストのメモリ解放 (ここが2KBの犯人の可能性大)
          resultMessages.clear();
          std::vector<String>().swap(resultMessages);

          // 4. 行動順リストの解放
          actorList.clear();
          std::vector<CombatantRef>().swap(actorList);
          if (!actualBossDefeated) {
            if (battleEndReason == BattleEndReason::DEFEAT) {
              bgm.emit(BattleBgmEvent::DEFEAT, currentBattleKind);
            }else {
              // 通常戦闘終了後: BGMを戻す
              bgm.emit(BattleBgmEvent::FINAL_FIELD_RETURN, currentBattleKind);
            }
          }
        }
        break;
      
    } // switch 終了
    if (!inBattle) {
      this->currentActor = std::nullopt;
      this->currentTarget = std::nullopt;
    } else {
      if (currentActor != originalActor) this->currentActor = refFor(currentActor);
      if (currentTarget != originalTarget) this->currentTarget = refFor(currentTarget);
    }
  }// update 終了

std::optional<BattleResult> Battle::takeResult() {
    return std::exchange(pendingResult, std::nullopt);
}

const CharacterState* Battle::currentActorState() const {
    if (!currentActor) return nullptr;
    switch (currentActor->side) {
      case CombatantSide::PLAYER:
        return &heroRef->state();
      case CombatantSide::ALLY:
        return &partyRef->member(currentActor->index).state();
      case CombatantSide::ENEMY:
        return &activeEnemies[currentActor->index].state();
    }
    return nullptr;
}

std::vector<const CharacterState*> Battle::targetableAlliesForRender(
    bool revive) const {
    std::vector<const CharacterState*> allies;
    if (revive) {
      if (heroRef && heroRef->state().stats().getHp() <= 0) {
        allies.push_back(&heroRef->state());
      }
      if (partyRef) {
        for (size_t i = 0; i < partyRef->memberCount(); ++i) {
          const CharacterState& ally = partyRef->member(i).state();
          if (ally.stats().getHp() <= 0) allies.push_back(&ally);
        }
      }
    } else {
      if (heroRef->state().stats().getHp() > 0) {
        allies.push_back(&heroRef->state());
      }
      for (size_t i = 0; i < partyRef->memberCount(); ++i) {
        const CharacterState& ally = partyRef->member(i).state();
        if (ally.stats().getHp() > 0) allies.push_back(&ally);
      }
    }
    return allies;
}

void Battle::ExecuteSkill(Skill& skill, CharacterState* user, CharacterState* target, AudioManager& music) {
    if (target == nullptr) return;
    const SkillAudioCue audio{&music, [](void* context, int seId) {
        static_cast<AudioManager*>(context)->playSE(seId);
    }};
    const SkillEffectResult result = SkillEffectSystem::execute(skill, user, target, audio);
    lastDamage += result.lastDamageDelta;
    if (result.actionMessage != nullptr) lastActionMessage = result.actionMessage;
}

std::vector<CharacterState*> Battle::getAliveAllies() {
    std::vector<CharacterState*> allies;
    // 主人公
    if (heroRef->state().stats().getHp() > 0) {
      allies.push_back(&heroRef->state());
    }
    // 仲間
    for (size_t i = 0; i < partyRef->memberCount(); ++i) {
      CharacterState& ally = partyRef->memberState(i);
      if (ally.stats().getHp() > 0) {
        allies.push_back(&ally);
      }
    }
    return allies;
  }

std::vector<CharacterState*> Battle::getDeadAllies() {
    std::vector<CharacterState*> allies;

    // 主人公
    if (heroRef &&
        heroRef->state().stats().getHp() <= 0) {
      allies.push_back(&heroRef->state());
    }

    // 仲間
    if (partyRef) {
      for (size_t i = 0; i < partyRef->memberCount(); ++i) {
        CharacterState& ally = partyRef->memberState(i);
        if (ally.stats().getHp() <= 0) {
          allies.push_back(&ally);
        }
      }
    }

    return allies;
  }

std::vector<CharacterState*> Battle::getAllAllies() {
    std::vector<CharacterState*> allies;

    if (heroRef) {
      allies.push_back(&heroRef->state());
    }

    if (partyRef) {
      for (size_t i = 0; i < partyRef->memberCount(); ++i) {
        allies.push_back(&partyRef->memberState(i));
      }
    }

    return allies;
  }

std::vector<CharacterState*> Battle::getAliveEnemies() {
    std::vector<CharacterState*> enemies;

    for (auto& enemy : activeEnemies) {
      if (enemy.state().stats().getHp() > 0) {
        enemies.push_back(&enemy.state());
      }
    }

    return enemies;
  }
