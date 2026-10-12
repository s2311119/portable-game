#pragma once
#include <Arduino.h>
#include "hardware/structs/sio.h"
#include "BgmCue.h"

static const int DAC_CS_PIN   = 17; // MCP4921 CS
static const int DAC_SCK_PIN  = 9;  // ソフトSPIクロック
static const int DAC_MOSI_PIN = 8;  // ソフトSPI MOSI

class Storage;
class AudioManager {
public:
  static const int BUF_SIZE = 4096;

  int currentTrack = 0;
  volatile bool pendingSwitch = false;
  int pendingTrack = -1;
  size_t refillOffset = 0;

  volatile uint8_t buffer0[BUF_SIZE];
  volatile uint8_t buffer1[BUF_SIZE];
  volatile uint8_t* playBuf = buffer0;
  volatile uint8_t* fillBuf = buffer1;

  volatile size_t playBytes = 0;
  volatile size_t fillBytes = 0;
  volatile size_t sampleIndex = 0;

  volatile bool needs_refill = true;
  volatile bool paused = false;
  // SEシンセ用の状態

  // 波形タイプ
  enum WaveType { OFF, SQUARE, NOISE };

  volatile WaveType seType = OFF; // 現在鳴っている音の種類
  volatile float sePhase = 0;     // 波形の進行度
  volatile float seFreq = 0;      // 周波数 (高さ)
  volatile float seFreqSlide = 0; // 周波数の変化量 (ピュイッ、のような効果)
  volatile float seVol = 0;       // 音量
  volatile float seVolDecay = 0;  // 音量の減衰量 (フェードアウト)
  volatile int targetSeVol = 10;
  float bgmVolume = 1.0;
  int masterSeVol = 10;
  void setSeVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 10) vol = 10;
    masterSeVol = vol;
  }
  AudioManager() {
    // DAC ピン初期化（core1 でもやるがここで safe）
    pinMode(DAC_CS_PIN, OUTPUT);
    pinMode(DAC_SCK_PIN, OUTPUT);
    pinMode(DAC_MOSI_PIN, OUTPUT);
    digitalWrite(DAC_CS_PIN, HIGH);
    // SE変数の初期化
    seType = OFF;
    sePhase = 0;
    seFreq = 0;
    seFreqSlide = 0;
    seVol = 0;
    seVolDecay = 0;
  }
  // AudioManager クラス内

  void playSE(int id) {
      sePhase = 0;
      // 設定値(0-10)を反映
      // masterSeVol が 10 なら 5000.0、5 なら 2500.0、0 なら 0.0
      float baseVol = 5000.0f * ((float)masterSeVol / 10.0f);
      seVol = baseVol;
      //seVol = 5000.0; // 基本音量

      switch (id) {
          case 0: // 決定 (ピロリン)
              seType = SQUARE;
              seFreq = 880;
              seFreqSlide = 0.5; // ほんの少し上がる
              seVolDecay = 4.0;  // 0.1秒くらいで消える
              break;

          case 1: // キャンセル (ブッ)
              seType = SQUARE;
              seFreq = 150;
              seFreqSlide = -0.1;
              seVolDecay = 8.0;   // 早めに消える
              break;

          case 2: // カーソル (ッ)
              seType = SQUARE;
              seFreq = 2000;
              seFreqSlide = 0;
              seVolDecay = 20.0; // 一瞬
              break;

          case 3: // 通常攻撃 (ザシュ)
              seType = NOISE;
              seFreq = 0;
              seFreqSlide = 0;
              seVolDecay = 2.0; // 0.25秒くらい
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          case 4: // 会心 (ギャイン！)
              seType = NOISE;
              seFreq = 0;
              seFreqSlide = 0;
              seVolDecay = 0.5; // 1秒くらい響
              seVol = 4000 * ((float)masterSeVol / 10.0f);
              break;

          case 5: // 物理技 (ドカッ)
              seType = SQUARE;
              seFreq = 100;
              seFreqSlide = -0.2;
              seVolDecay = 1.0;
              break;

          case 6: // 物理全体 (ザーッ)
              seType = NOISE;
              seFreq = 0;
              seFreqSlide = 0;
              seVolDecay = 1.0; // 長め
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          case 7: // 魔法/回復 (ヒュン)
              seType = SQUARE;
              seFreq = 600;
              seFreqSlide = 2.0; // キュイーンと上がる
              seVolDecay = 2.5;
              break;

          case 8: // 魔法全体 (ジャーン)
              seType = NOISE;
              seFreq = 0;
              seFreqSlide = 0;
              seVolDecay = 1.5;
              seVol = 2000 * ((float)masterSeVol / 10.0f);
              break;

          default:
              seType = OFF;
              break;
      }
  }
  // 最速 bit-bang 用（ISR から呼ぶ。gpio_put を使う）
  inline void softSPIWrite_fast(uint16_t value) {
    uint32_t mask = (1u << DAC_MOSI_PIN);
    uint32_t clk  = (1u << DAC_SCK_PIN);
    for (int i = 15; i >= 0; --i) {
      if (value & (1 << i))
        sio_hw->gpio_set = mask;
      else
        sio_hw->gpio_clr = mask;
      sio_hw->gpio_set = clk;
      sio_hw->gpio_clr = clk;
    }
  }

  inline void setDAC_ISR(uint16_t value) {
    uint16_t dac_data = 0x3000 | (value & 0x0FFF);
    uint32_t csMask = (1u << DAC_CS_PIN);
    sio_hw->gpio_clr = csMask;
    softSPIWrite_fast(dac_data);
    sio_hw->gpio_set = csMask;
  }

  void playBGM(BgmCue cue);
  void initializeTracks(class Storage &sd);
  void performPendingSwitch(class Storage &sd);
  void refile(class Storage &sd);
  void runWhileStoragePaused(void* context, void (*operation)(void*));
  void beginStorageOperation();
  void endStorageOperation();

private:
  void requestSwitch(int trackId);
  void switchTrack(int newId);
};
