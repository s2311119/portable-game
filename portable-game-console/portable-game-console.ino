// main.ino
#include "Game.h"
#include <SPI.h>
#include "LGFX_RP2040_ILI9341.hpp"
#include <LGFX_TFT_eSPI.hpp>  // フォントを使う場合に必要な場合もあります
#include "bmp.h"
#include "Item.h"
#include "ItemCatalog.h"
#include "CharacterState.h"
#include "CharacterProgressionSystem.h"
#include "CombatCalculator.h"
#include "SkillEffectSystem.h"
#include "Automaton.h"
#include "AutomatonStorage.h"
#include "Party.h"
#include "Enemy.h"
#include "Player.h"
#include "Graphic.h"
#include "WorldRenderer.h"
#include "TitleRenderer.h"
#include "GameOverRenderer.h"
#include "LegacyGameTypes.h"
#include "Controller.h"
#include "EncounterManager.h"
#include "EncounterSpec.h"
#include "Exploration.h"
#include "FieldEventHandler.h"
#include "Vibration.h"
#include "Skill.h"
#include "SkillCatalog.h"
#include "Inventory.h"
#include "Map.h"
#include "WorldProgress.h"
#include "FloorManager.h"
#include "Settings.h"
#include "AudioManager.h"
#include "BgmCue.h"
#include "Storage.h"
#include "SaveManager.h"
#include "ImageManager.h"
#include "GameSession.h"
#include "Battle.h"
#include "BattleRenderer.h"
#include "MenuFlow.h"
#include "MainMenuRenderer.h"
#include "ItemRenderer.h"
#include "SkillRenderer.h"
#include "EquipmentRenderer.h"
#include "EvolutionRenderer.h"
#include "ShopRenderer.h"
#include "InnRenderer.h"
#include "StorageRenderer.h"
#include "SettingsRenderer.h"
#include "MemoryMonitor.h"
#include <utility>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/gpio.h"


#define LGFX_USE_SD 1
#define H_TRANSPARENT 0xFFFF
// -------------------------
// Map / Game / Graphic 等（省略せずそのまま）
// -------------------------
// ここは質問者が既に持っているゲームロジック部分をそのまま使います。
// （読みやすさのため必要最小限のみ再掲します）
// main.ino

#include <vector>      // std::vector を使うために必要
#include <algorithm>   // std::min, std::max を使うために必要

//-----------------------------
/*
ストライカー系: 39->44->49->{54, 55} ->{{61, 62}, {63, 64}}
ガーディアン系: 40->45->50->{56, 57} ->{{65, 66}, {67. 68}}
クリッパー系: 41->46->51->58 ->{69, 70}
メンダ―系: 42->47->52->59 ->{71, 72}
ジャマー系: 43->48->53->60->{73, 74}
*/
//-----------------------------

// グローバル（core0 から参照するけど SD 操作は core1 のみ）





// main.ino


// main.ino の Item クラスの後に追加












// -------------------------
// Graphic クラス (省メモリ・直接描画版)
// -------------------------





// -------------------------
// AUDIO / SD (core1 に限定)
// -------------------------


// ---------
// saveクラス
// ---------



// -------------------------
// BATTLE クラス
// -------------------------


Game* ctx = nullptr;

// タイマー割り込み関数

bool Game::audioTimerCallback(struct repeating_timer *t) {
  AudioManager* music = ctx->music;
  
  // BGM停止中でもSEは鳴らす
  int32_t mixedSample = 0;

  // --- 1. BGMの取得 ---
  if (!music->paused) {

    // 現在の再生バッファを使い切っていたら、
    // 先に裏バッファへ切り替える
    if (music->sampleIndex + 1 >= music->playBytes) {

      if (music->fillBytes > 0) {

        // ここはタイマーISR内なので、
        // noInterrupts()/interrupts() は不要
        volatile uint8_t* tmp = music->playBuf;
        music->playBuf = music->fillBuf;
        music->fillBuf = (uint8_t*)tmp;

        music->playBytes = music->fillBytes;
        music->fillBytes = 0;
        music->sampleIndex = 0;

        // 空いた裏バッファの補充をcore1ループへ要求
        music->needs_refill = true;
      }
      else {
        // 裏バッファの準備が間に合っていない
        music->needs_refill = true;
      }
    }

    // バッファを切り替えた場合も、
    // 同じタイマー割り込み内でそのまま次の1サンプルを読む。
    // これによりバッファ境界に無音サンプルを挟まない。
    if (music->sampleIndex + 1 < music->playBytes) {

      uint8_t lo =
          music->playBuf[music->sampleIndex];

      uint8_t hi =
          music->playBuf[music->sampleIndex + 1];

      int16_t bgm =
          (int16_t)(lo | (hi << 8));

      bgm =
          (int16_t)(bgm * music->bgmVolume);

      mixedSample += bgm;

      music->sampleIndex += 2;
    }
  }

  // --- 2. SE (シンセサイザー生成) ★ここが変わった！ ---
  if (music->seType != AudioManager::OFF) {
    int16_t seWave = 0;

    // 音量の減衰
    music->seVol -= music->seVolDecay;
    if (music->seVol <= 0) {
      music->seType = AudioManager::OFF; // 音量0で終了
      music->seVol = 0;
    } else {
      // 周波数の変化 (スライド)
      music->seFreq += music->seFreqSlide;
      
      // 波形生成
      if (music->seType == AudioManager::SQUARE) {
        // 矩形波: 位相を進めて High/Low を切り替え
        // 16000Hz動作なので、位相は Freq / 16000 ずつ進む
        music->sePhase += music->seFreq / 16000.0f;
        if (music->sePhase >= 1.0f) music->sePhase -= 1.0f;
        
        seWave = (music->sePhase < 0.5f) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      } 
      else if (music->seType == AudioManager::NOISE) {
        // ノイズ: 乱数
        // (高速化のため簡易的な乱数生成)
        static uint16_t lfsr = 0xACE1;
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        seWave = (lfsr & 1) ? (int16_t)music->seVol : (int16_t)-music->seVol;
      }
    }
    
    mixedSample += seWave;
  }

  // --- 3. クリッピング (音割れ防止) ---
  if (mixedSample > 32767) mixedSample = 32767;
  if (mixedSample < -32768) mixedSample = -32768;

  // --- 4. DACへ出力 ---
  // 16bit signed -> 12bit unsigned (0〜4095)
  uint16_t usample = (uint16_t)(mixedSample + 32768) >> 6;
  if (usample > 4095) usample = 4095;
  music->setDAC_ISR(usample);
  
  return true;
}

// -------------------------
// core1 のエントリポイント（オーディオ専用）
// -------------------------
void Game::core1Main() {

  // -------------------------------------------------
  // 音声専用タイマーをcore1上に作成
  //
  // デフォルトalarm poolは使わず、
  // このcore1上で独自のalarm poolを作ることで、
  // audio_timer_callbackもcore1側のIRQとして実行する。
  // -------------------------------------------------
  static alarm_pool_t* audioAlarmPool =
      alarm_pool_create_with_unused_hardware_alarm(1);

  static repeating_timer_t audioTimer;

  // 16kHzの理想周期は62.5us。
  // repeating_timerは整数us指定なので、
  // 近い63usを使用する。
  //
  // 負値にすることで
  // 「callback終了から63us後」ではなく
  // 「callback開始から63usごと」に呼ばれる。
  bool timerStarted =
      alarm_pool_add_repeating_timer_us(
          audioAlarmPool,
          -63,
          Game::audioTimerCallback,
          nullptr,
          &audioTimer
      );

  // タイマー生成に失敗した場合は、
  // 不定状態でゲームを続けない。
  if (!timerStarted) {
    while (true) {
      tight_loop_contents();
    }
  }

  while (true) {
    ctx->music->performPendingSwitch(*ctx->sd);
    ctx->music->refile(*ctx->sd);

    tight_loop_contents();
  }
}

// main.ino (setup 関数)

void setup() {
  Game::boot();
}

void loop() {
  ctx->update();
}