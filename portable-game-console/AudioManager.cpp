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

  sd.requestSwitch(newId);
}
