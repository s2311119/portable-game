#include "BattleRenderer.h"

#include "Battle.h"
#include "Graphic.h"
#include "ImageManager.h"
#include "SkillCatalog.h"
#include "bmp.h"

#include <vector>

#define H_TRANSPARENT 0xFFFF

void BattleRenderer::draw(const Battle& battle, const Player& hero,
                          const Party& party, const ImageManager& images,
                          Graphic& graphic) const {
    if (!battle.inBattle) return;
    Graphic* g = &graphic;
    const CharacterState* currentActor = battle.currentActorState();
    const auto currentBattleState = battle.currentBattleState;
    const auto statusPage = battle.statusPage;
    const auto& activeEnemies = battle.activeEnemies;
    const auto enemyTargetIndex = battle.enemyTargetIndex;
    const auto allyTargetIndex = battle.allyTargetIndex;
    const auto pendingActionType = battle.pendingActionType;
    const auto pendingSkillIndex = battle.pendingSkillIndex;
    const auto commandSelected = battle.commandSelected;
    const auto skillSelected = battle.skillSelected;
    const auto& commands = battle.commands;
    const auto& message = battle.message;

    // --- 分割描画ループ ---
    // Graphic::BLOCK_HEIGHT (32) を使う
    for (int drawY = 0; drawY < 240; drawY += Graphic::BLOCK_HEIGHT) {

      g->lineSprite.fillScreen(TFT_BLACK); // sp. -> g->lineSprite.

      // 1. Yボタン（ステータス確認）の場合
      if (currentBattleState == Battle::STATE_CHECK_STATUS) {
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setTextSize(2);

        const CharacterState* s = nullptr;
        if (statusPage == 0) s = &hero.state();
        else if (statusPage - 1 < party.memberCount()) s = &party.member(statusPage - 1).state();

        if (s != nullptr) {
          // 名前とレベル
          g->lineSprite.setCursor(10, 10 - drawY);
          g->lineSprite.printf("%s Lv%d", s->name(), s->progress().level());

          // HP / TP
          g->lineSprite.setCursor(10, 40 - drawY);
          g->lineSprite.printf("HP: %d / %d", s->stats().getHp(), s->stats().getMaxHp());
          g->lineSprite.setCursor(10, 65 - drawY);
          g->lineSprite.printf("TP: %d / %d", s->stats().getTp(), s->stats().getMaxTp());

          // 能力値
          g->lineSprite.setTextSize(1.5);
          int col1 = 10; int col2 = 120;
          int startY = 100; int gap = 25;

          g->lineSprite.setCursor(col1, startY - drawY);
          g->lineSprite.printf("ATK: %d", s->attack());
          g->lineSprite.setCursor(col2, startY - drawY);
          g->lineSprite.printf("DEF: %d", s->defense());

          g->lineSprite.setCursor(col1, startY + gap - drawY);
          g->lineSprite.printf("SPD: %d", s->speed());
          g->lineSprite.setCursor(col2, startY + gap - drawY);
          g->lineSprite.printf("SEN: %d", s->sense());

          g->lineSprite.setCursor(col1, startY + gap*2 - drawY);
          g->lineSprite.printf("LCK: %d", s->luck());

          g->lineSprite.setCursor(col2, startY + gap*2 - drawY);
          g->lineSprite.printf("EXP: %d", s->progress().xp());

          // ページガイド
          g->lineSprite.setTextSize(2);
          g->lineSprite.setTextColor(TFT_YELLOW);
          g->lineSprite.setCursor(140, 210 - drawY);
          g->lineSprite.print("<   >");
        }
      }

      // 2. 通常戦闘画面の場合
      else {
        // 背景
        g->lineSprite.pushImage(0, 0 - drawY, Graphic::SCREEN_WIDTH, Graphic::SCREEN_HEIGHT, (uint16_t*)battle_back);

        // --- 敵パーティー描画 ---
        const int enemyX[] = {10, 60, 10, 60};
        const int enemyY[] = {20, 40, 90, 110}; // 少しジグザグ配置
        for (int i = 0; i < activeEnemies.size(); ++i) {
          if (i >= 4) break;
          const Enemy& enemy = activeEnemies[i];

          if (enemy.state().stats().getHp() > 0 || !enemy.state().popupValues.empty()) {
            int ex = enemyX[i];
            int ey = enemyY[i] - drawY;

            if (enemy.state().stats().getHp() > 0) {
              const uint16_t* img = images.enemyImage(i);
              g->lineSprite.fillEllipse(ex + 32, ey + 60, 24, 8, TFT_RED);
              if (img) g->lineSprite.pushImage(ex, ey, 64, 64, const_cast<uint16_t*>(img), H_TRANSPARENT);
              else g->lineSprite.fillRect(ex, ey, 64, 64, TFT_RED);
            }

            // カーソル (敵)
            if (currentBattleState == Battle::STATE_TARGET_SELECT_ENEMY && enemyTargetIndex == i && enemy.state().stats().getHp() > 0) {
              g->lineSprite.setTextColor(TFT_YELLOW); g->lineSprite.setTextSize(2);
              g->lineSprite.setCursor(ex + 26, ey - 20); g->lineSprite.print("▼");
            }
            // ポップアップ (敵)
            if (!enemy.state().popupValues.empty()) {
              int pY = ey - 20; // 少し上から
              g->lineSprite.setTextSize(2);
              for (const auto& data : enemy.state().popupValues) {
                int val = data.value;
                bool isCrit = data.isCritical; // ★ 会心チェック

                if (val == 0) {
                  g->lineSprite.setTextColor(TFT_LIGHTGREY);
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print("Miss");
                } else if (val > 0) { // ダメージ
                  if (isCrit) {
                     g->lineSprite.setTextColor(TFT_RED); // 会心は赤
                  } else {
                     g->lineSprite.setTextColor(TFT_YELLOW); // 通常は黄色
                  }
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print(val);
                } else { // 回復
                  g->lineSprite.setTextColor(TFT_GREEN);
                  g->lineSprite.setCursor(ex + 10, pY); g->lineSprite.print(-val);
                }
                pY -= 20; // 間隔広め
              }
              g->lineSprite.setTextSize(1.5); // 戻す
            }
          }
        }

        // --- 味方パーティー描画 ---
        const int heroX = 240;
        const int heroY = 20 - drawY; // 右上

        // 主人公
        if (hero.state().stats().getHp() > 0 || !hero.state().popupValues.empty()) {
          if (hero.state().stats().getHp() > 0) {
            const uint16_t* img = images.playerImage();
            g->lineSprite.fillEllipse(heroX + 32, heroY + 60, 24, 8, TFT_CYAN);
            if (img) g->lineSprite.pushImage(heroX, heroY, 64, 64, const_cast<uint16_t*>(img), H_TRANSPARENT);
            else g->lineSprite.fillRect(heroX, heroY, 64, 64, TFT_BLUE);
          }
          // ポップアップ
          if (!hero.state().popupValues.empty()) {
            int pY = heroY - 20;
            g->lineSprite.setTextSize(2);
            for (const auto& data : hero.state().popupValues) {
              int val = data.value;
              bool isCrit = data.isCritical;
              if (val == 0) {
                g->lineSprite.setTextColor(TFT_LIGHTGREY);
                g->lineSprite.setCursor(heroX + 10, pY);
                g->lineSprite.print("Miss");
              }else if (val > 0) {
                // 会心なら赤、通常なら黄色
                if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                else g->lineSprite.setTextColor(TFT_YELLOW);
                g->lineSprite.setCursor(heroX + 10, pY);
                g->lineSprite.print(val);
              }
              else {
                g->lineSprite.setTextColor(TFT_GREEN);
                g->lineSprite.setCursor(heroX + 10, pY);
                g->lineSprite.print(-val);
              }
              pY -= 20;
            }
            g->lineSprite.setTextSize(1.5);
          }
        }

        // 仲間
        const int allyX[] = {190, 240, 190};
        const int allyY_base[] = {60, 90, 120}; // 右下
        for (int i = 0; i < party.memberCount(); i++) {
          const CharacterState& ally = party.member(i).state();
          int ay = allyY_base[i] - drawY;

          if (ally.stats().getHp() > 0 || !ally.popupValues.empty()) {
            if (ally.stats().getHp() > 0) {
              const uint16_t* img = images.allyImage(i);
              g->lineSprite.fillEllipse(allyX[i] + 32, ay + 60, 24, 8, TFT_CYAN);
              if (img) g->lineSprite.pushImage(allyX[i], ay, 64, 64, const_cast<uint16_t*>(img), H_TRANSPARENT);
              else g->lineSprite.fillRect(allyX[i], ay, 64, 64, TFT_GREEN);
            }
            // ポップアップ
            if (!ally.popupValues.empty()) {
              int pY = ay - 20;
              g->lineSprite.setTextSize(2);
              for (const auto& data : ally.popupValues) {
                int val = data.value;
                bool isCrit = data.isCritical;
                if (val == 0) {
                  g->lineSprite.setTextColor(TFT_LIGHTGREY);
                  g->lineSprite.setCursor(allyX[i] + 20, pY);
                  g->lineSprite.print("Miss");
                }else if (val > 0) {
                  // 会心なら赤、通常なら黄色
                  if (isCrit) g->lineSprite.setTextColor(TFT_RED);
                  else g->lineSprite.setTextColor(TFT_YELLOW);
                  g->lineSprite.setCursor(allyX[i] + 20, pY);
                  g->lineSprite.print(val);
                }
                else {
                  g->lineSprite.setTextColor(TFT_GREEN);
                  g->lineSprite.setCursor(allyX[i] + 20, pY);
                  g->lineSprite.print(-val);
                }
                pY -= 20;
              }
              g->lineSprite.setTextSize(1.5);
            }
          }
        }

        // ターゲットカーソル(味方)
        if (currentBattleState == Battle::STATE_TARGET_SELECT_ALLY) {
          bool isReviveSkill = false;

          if (currentActor &&
              pendingActionType == Battle::ActionType::SKILL) {

            const SkillSet& skills =
                currentActor->skills();

            if (pendingSkillIndex >= 0 &&
                pendingSkillIndex < static_cast<int>(skills.count())) {
              isReviveSkill =
                  (SkillCatalog::get(skills.skillIdAt(pendingSkillIndex)).type ==
                  Skill::EffectType::REVIVE);
            }
          }

          std::vector<const CharacterState*> allies =
              battle.targetableAlliesForRender(isReviveSkill);

          if (allyTargetIndex < allies.size()) {
            const CharacterState* targetedAlly = allies[allyTargetIndex];
            int tx = 0, ty = 0;
            if (targetedAlly == &hero.state()) { tx = 220; ty = 20; }
            else {
              for (int i = 0; i < party.memberCount(); i++) {
                if (&party.member(i).state() == targetedAlly) {
                  tx = allyX[i]; ty = allyY_base[i];
                  break;
                }
              }
            }
            if (tx != 0) {
              g->lineSprite.setTextColor(TFT_YELLOW);
              g->lineSprite.setTextSize(2);
              g->lineSprite.setCursor(tx + 26, ty - 20 - drawY); // 位置調整
              g->lineSprite.print("▼");
            }
          }
        }

        // --- コマンドウィンドウ (小型化・中央下) ---
        int cmdX = 80;
        int cmdY = 170 - drawY;
        int cmdW = 160;
        int cmdH = 65;

        g->lineSprite.fillRect(cmdX, cmdY, cmdW, cmdH, TFT_DARKGREY);
        g->lineSprite.drawRect(cmdX, cmdY, cmdW, cmdH, TFT_WHITE);
        g->lineSprite.setTextSize(1);

        if (currentBattleState == Battle::STATE_COMMAND_SELECT) {
          int x_offset = cmdX + 10;
          int y_offset = cmdY + 10;
          for (int i = 0; i < Battle::CMD_COUNT; i++) {
            uint16_t color = (i == commandSelected) ? TFT_YELLOW : TFT_WHITE;
            g->lineSprite.setTextColor(color);
            g->lineSprite.setCursor(x_offset, y_offset);
            g->lineSprite.print(commands[i]);
            if(i % 2 == 0) x_offset += 70;
            if(i % 2 == 1){ y_offset += 25; x_offset -= 70; }
          }
        }
        else if (currentBattleState == Battle::STATE_SKILL_SELECT) {
          if (currentActor) {
            const SkillSet& skills = currentActor->skills();
            int skillCount = skills.count();
            // スクロール計算
            int itemsPerPage = 3;
            int startIdx = 0;
            if (skillSelected >= itemsPerPage) {
              startIdx = skillSelected - (itemsPerPage - 1);
            }
            int endIdx = startIdx + itemsPerPage;
            if (endIdx > skillCount) endIdx = skillCount;

            for (int i = startIdx; i < endIdx; i++) {
              uint16_t color = (i == skillSelected) ? TFT_YELLOW : TFT_WHITE;
              g->lineSprite.setTextColor(color);
              int relativeIndex = i - startIdx;
              g->lineSprite.setCursor(cmdX + 10, cmdY + 5 + (relativeIndex * 20));
              g->lineSprite.printf("%s(%d)", SkillCatalog::get(skills.skillIdAt(i)).name, SkillCatalog::get(skills.skillIdAt(i)).tpCost);
            }
          }
        }

        // --- メッセージ (最下部に移動) ---
        // メッセージをキャラと重ならない位置に表示
        g->lineSprite.setTextSize(1);
        g->lineSprite.setTextColor(TFT_WHITE);
        g->lineSprite.setCursor(10, 220 - drawY); // ここ！
        g->lineSprite.print(message);
      }

      // 画面に転送
      g->lineSprite.pushSprite(0, drawY);
    }
}
