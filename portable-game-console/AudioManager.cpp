#include "Storage.h"
#include "StorageSyncInternal.h"

void AudioManager::playBGM(BgmCue cue, Storage &sd) {
  int track = 0;
  switch (cue) {
    case BgmCue::TOWN: track = 0; break;
    case BgmCue::DUNGEON: track = 1; break;
    case BgmCue::BATTLE_NORMAL: track = 2; break;
    case BgmCue::BATTLE_BOSS: track = 3; break;
    case BgmCue::BATTLE_FINAL: track = 4; break;
    case BgmCue::TITLE: track = 9; break;
    case BgmCue::GAME_OVER: track = 99; break;
  }
  switchTrack(track, sd);
}

void AudioManager::initializeTracks(Storage &sd) {
  const char* paths[TRACK_COUNT];
  for(int i=0; i<TRACK_COUNT; i++) paths[i] = "";
  paths[0] = "/bgm_0.raw"; // 集落
  paths[1] = "/bgm_1.raw"; // ダンジョン
  paths[2] = "/bgm_2.raw"; // 通常戦闘
  paths[3] = "/bgm_3.raw"; // ボス戦
  paths[4] = "/bgm_4.raw"; // ラスボス
  paths[9] = "/bgm_9.raw"; // タイトル
  paths[99] = "/bgm_99.raw";
  for (int i = 0; i < TRACK_COUNT; ++i) {
    sd.openTrack(i, paths[i]);
    if (!sd.trackIsOpen(i)) {
      //Serial.printf("Failed to open %s\n", paths[i]);
      //while (1);
    }
  }
  currentTrack = 9;
  // 初期曲データ読み込み
  sd.seekTrackStart(currentTrack);
  size_t n0 = sd.readTrack(currentTrack, (void*)buffer0, BUF_SIZE);
  size_t n1 = sd.readTrack(currentTrack, (void*)buffer1, BUF_SIZE);
  noInterrupts();
  playBuf = buffer0;
  fillBuf = buffer1;
  playBytes = n0;
  fillBytes = n1;
  sampleIndex = 0;
  needs_refill = false;
  interrupts();
}

void AudioManager::requestSwitch(int trackId) {
    if (pendingSwitch) return;
    noInterrupts();
    pendingTrack = trackId;
    pendingSwitch = true;
    interrupts();
  }

void AudioManager::performPendingSwitch(Storage &sd) {
    if (!pendingSwitch) return;

    // --- 競合防止ロック ---
    critical_section_enter_blocking(&sdLock);

    int newTrack;
    noInterrupts();
    newTrack = pendingTrack;
    pendingSwitch = false;
    interrupts();

    paused = true;

    // --- バッファ完全リセット ---
    memset((void*)buffer0, 0, BUF_SIZE);
    memset((void*)buffer1, 0, BUF_SIZE);
    sampleIndex = 0;
    playBytes = 0;
    fillBytes = 0;
    needs_refill = false;
    refillOffset = 0;

    // --- ファイルを最初から ---
    sd.seekTrackStart(newTrack);

    // --- 新しいデータをロード ---
    size_t n0 = sd.readTrack(newTrack, (void*)buffer0, BUF_SIZE);
    size_t n1 = sd.readTrack(newTrack, (void*)buffer1, BUF_SIZE);

    // --- 原子的にスイッチ ---
    noInterrupts();
    currentTrack = newTrack;
    playBuf = buffer0;
    fillBuf = buffer1;
    playBytes = n0;
    fillBytes = n1;
    sampleIndex = 0;
    needs_refill = false;
    interrupts();

    // --- ロック解除 ---
    critical_section_exit(&sdLock);

    delay(30);  // ISRが落ち着く時間
    paused = false;
  }

void AudioManager::refile(Storage &sd) {
    // 補充要求がないなら何もしない
    if (!needs_refill) {
      return;
    }

    // 曲切り替え中などはSD読み込みしない
    if (paused) {
      return;
    }

    if (!sd.trackFileIsOpen(currentTrack)) {
      return;
    }

    critical_section_enter_blocking(&sdLock);

    // 一度に長時間SDを占有しないよう、
    // 2048 byteずつ裏バッファへ補充する
    const size_t CHUNK = 2048;

    if (refillOffset < BUF_SIZE) {

      size_t remaining =
          BUF_SIZE - refillOffset;

      size_t toRead =
          min(CHUNK, remaining);

      size_t n =
          sd.readTrack(
              currentTrack,
              (void*)(fillBuf + refillOffset),
              toRead
          );

      refillOffset += n;

      // 要求量より少なくしか読めなかった場合は
      // 曲末尾に到達したとみなし、次回は先頭から続ける。
      //
      // 途中まで読めたデータは捨てず、
      // fillBuf内にそのまま残しておく。
      if (n < toRead) {
        sd.seekTrackStart(currentTrack);
      }
    }

    critical_section_exit(&sdLock);

    // 裏バッファ1個分が完成したら再生側へ公開
    if (refillOffset >= BUF_SIZE) {

      noInterrupts();

      fillBytes =
          BUF_SIZE;

      needs_refill =
          false;

      interrupts();

      refillOffset = 0;
    }
  }

// --- AudioManager::switchTrack 実装部（Storageの定義後） ---
void AudioManager::switchTrack(int newId, Storage &sd) {
  if (newId == currentTrack) return;

  // --- 安全な切替要求 ---
  paused = true;

  // SDロックを取得（refileと競合しないようにする）
  critical_section_enter_blocking(&sdLock);
  sampleIndex = 0;
  needs_refill = false;
  critical_section_exit(&sdLock);

  requestSwitch(newId);
}
