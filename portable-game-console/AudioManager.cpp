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
