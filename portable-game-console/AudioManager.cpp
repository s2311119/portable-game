#include "Storage.h"

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
