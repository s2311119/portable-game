#pragma once
#include "AudioManager.h"
#include <algorithm>
#include <cstring>

static const int SD_CS_PIN = 22;    // SD カード CS
static const int SD_MISO_PIN = 16;  // SD MISO（プルアップ必須）
static const int SD_SCK_PIN  = 18;
static const int SD_MOSI_PIN = 19;

class Storage {
public:
  static void initializeSynchronization();
  volatile bool pendingSwitch = false;
  int pendingTrack = -1;
  size_t refillOffset = 0;
private:
  SdFat sdfat;
public:
  Storage();
  void resetRefillOffset();
  void init(AudioManager &music);

  // 切り替え予約（ID指定）
  void requestSwitch(int trackId);

  // core1ループで実行
  void performPendingSwitch(AudioManager &music);

  void refile(AudioManager &music);

  bool loadToBuffer(String filename, uint16_t* buffer);
  FsFile openFile(const char* filename, uint8_t flags);
  bool existsFile(const char* filename);
  void runWhileAudioPaused(AudioManager& music, void* context,
                           void (*operation)(void*));
  void beginSynchronizedWrite(AudioManager& music);
  void endSynchronizedWrite(AudioManager& music);
};
