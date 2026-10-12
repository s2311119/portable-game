#pragma once
#include "AudioManager.h"
#include "pico/sync.h"
#include <algorithm>
#include <cstring>

extern critical_section_t sdLock;

static const int SD_CS_PIN = 22;    // SD カード CS
static const int SD_MISO_PIN = 16;  // SD MISO（プルアップ必須）
static const int SD_SCK_PIN  = 18;
static const int SD_MOSI_PIN = 19;

class Sd {
public:
  volatile bool pendingSwitch = false;
  int pendingTrack = -1;
  size_t refillOffset = 0;
  SdFat sdfat;
  Sd() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
  }
  void resetRefillOffset() { refillOffset = 0; }
  void init(AudioManager &music) {
    if (!sdfat.begin(SD_CS_PIN, SD_SCK_MHZ(20))) {
      //Serial.println("SD init failed!");
      while (1);
    }

    const char* paths[AudioManager::TRACK_COUNT];
    for(int i=0; i<AudioManager::TRACK_COUNT; i++) paths[i] = "";
    paths[0] = "/bgm_0.raw"; // 集落
    paths[1] = "/bgm_1.raw"; // ダンジョン
    paths[2] = "/bgm_2.raw"; // 通常戦闘
    paths[3] = "/bgm_3.raw"; // ボス戦
    paths[4] = "/bgm_4.raw"; // ラスボス
    paths[9] = "/bgm_9.raw"; // タイトル
    paths[99] = "/bgm_99.raw";
    for (int i = 0; i < AudioManager::TRACK_COUNT; ++i) {
      music.musicFiles[i] = sdfat.open(paths[i], FILE_READ);
      if (!music.musicFiles[i]) {
        //Serial.printf("Failed to open %s\n", paths[i]);
        //while (1);
      }
    }
    music.currentTrack = 9;
    // 初期曲データ読み込み
    FsFile& f = music.musicFiles[music.currentTrack];
    f.seek(0);
    size_t n0 = f.read((void*)music.buffer0, AudioManager::BUF_SIZE);
    size_t n1 = f.read((void*)music.buffer1, AudioManager::BUF_SIZE);
    noInterrupts();
    music.playBuf = music.buffer0;
    music.fillBuf = music.buffer1;
    music.playBytes = n0;
    music.fillBytes = n1;
    music.sampleIndex = 0;
    music.needs_refill = false;
    interrupts();
  }

  // 切り替え予約（ID指定）
  void requestSwitch(int trackId) {
    if (pendingSwitch) return;
    noInterrupts();
    pendingTrack = trackId;
    pendingSwitch = true;
    interrupts();
  }

  // core1ループで実行
  void performPendingSwitch(AudioManager &music) {
    if (!pendingSwitch) return;

    // --- 競合防止ロック ---
    critical_section_enter_blocking(&sdLock);

    int newTrack;
    noInterrupts();
    newTrack = pendingTrack;
    pendingSwitch = false;
    interrupts();

    music.paused = true;

    // --- バッファ完全リセット ---
    memset((void*)music.buffer0, 0, AudioManager::BUF_SIZE);
    memset((void*)music.buffer1, 0, AudioManager::BUF_SIZE);
    music.sampleIndex = 0;
    music.playBytes = 0;
    music.fillBytes = 0;
    music.needs_refill = false;
    refillOffset = 0;

    // --- ファイルを最初から ---
    FsFile &file = music.musicFiles[newTrack];
    file.seek(0);

    // --- 新しいデータをロード ---
    size_t n0 = file.read((void*)music.buffer0, AudioManager::BUF_SIZE);
    size_t n1 = file.read((void*)music.buffer1, AudioManager::BUF_SIZE);

    // --- 原子的にスイッチ ---
    noInterrupts();
    music.currentTrack = newTrack;
    music.playBuf = music.buffer0;
    music.fillBuf = music.buffer1;
    music.playBytes = n0;
    music.fillBytes = n1;
    music.sampleIndex = 0;
    music.needs_refill = false;
    interrupts();

    // --- ロック解除 ---
    critical_section_exit(&sdLock);

    delay(30);  // ISRが落ち着く時間
    music.paused = false;
  }

  void refile(AudioManager &music) {
    // 補充要求がないなら何もしない
    if (!music.needs_refill) {
      return;
    }

    // 曲切り替え中などはSD読み込みしない
    if (music.paused) {
      return;
    }

    FsFile& file =
        music.musicFiles[music.currentTrack];

    if (!file.isOpen()) {
      return;
    }

    critical_section_enter_blocking(&sdLock);

    // 一度に長時間SDを占有しないよう、
    // 2048 byteずつ裏バッファへ補充する
    const size_t CHUNK = 2048;

    if (refillOffset < AudioManager::BUF_SIZE) {

      size_t remaining =
          AudioManager::BUF_SIZE - refillOffset;

      size_t toRead =
          min(CHUNK, remaining);

      size_t n =
          file.read(
              (void*)(music.fillBuf + refillOffset),
              toRead
          );

      refillOffset += n;

      // 要求量より少なくしか読めなかった場合は
      // 曲末尾に到達したとみなし、次回は先頭から続ける。
      //
      // 途中まで読めたデータは捨てず、
      // fillBuf内にそのまま残しておく。
      if (n < toRead) {
        file.seek(0);
      }
    }

    critical_section_exit(&sdLock);

    // 裏バッファ1個分が完成したら再生側へ公開
    if (refillOffset >= AudioManager::BUF_SIZE) {

      noInterrupts();

      music.fillBytes =
          AudioManager::BUF_SIZE;

      music.needs_refill =
          false;

      interrupts();

      refillOffset = 0;
    }
  }

  bool loadToBuffer(String filename, uint16_t* buffer);
};
