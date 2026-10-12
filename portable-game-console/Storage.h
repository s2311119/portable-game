#pragma once
#include <Arduino.h>
#include <SdFat.h>
#include <algorithm>
#include <cstring>

static const int SD_CS_PIN = 22;    // SD カード CS
static const int SD_MISO_PIN = 16;  // SD MISO（プルアップ必須）
static const int SD_SCK_PIN  = 18;
static const int SD_MOSI_PIN = 19;

class Storage {
public:
  static const int TRACK_COUNT = 100;
  static void initializeSynchronization();
private:
  SdFat sdfat;
  FsFile musicFiles[TRACK_COUNT];
public:
  Storage();
  void init();
  void openTrack(int trackId, const char* path);
  bool trackIsOpen(int trackId) const;
  void seekTrackStart(int trackId);
  size_t readTrack(int trackId, void* buffer, size_t bytes);
  bool trackFileIsOpen(int trackId) const;

  bool loadToBuffer(String filename, uint16_t* buffer);
  FsFile openFile(const char* filename, uint8_t flags);
  bool existsFile(const char* filename);
};
