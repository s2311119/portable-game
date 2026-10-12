#include "Storage.h"
#include "pico/sync.h"

static critical_section_t sdLock;

void Storage::initializeSynchronization() {
    critical_section_init(&sdLock);
  }

void Storage::lockIo() {
    critical_section_enter_blocking(&sdLock);
  }

void Storage::unlockIo() {
    critical_section_exit(&sdLock);
  }

Storage::Storage() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
  }

void Storage::init() {
    if (!sdfat.begin(SD_CS_PIN, SD_SCK_MHZ(20))) {
      //Serial.println("SD init failed!");
      while (1);
    }
  }

void Storage::openTrack(int trackId, const char* path) {
    musicFiles[trackId] = sdfat.open(path, FILE_READ);
  }

bool Storage::trackIsOpen(int trackId) const {
    return (bool)musicFiles[trackId];
  }

void Storage::seekTrackStart(int trackId) {
    musicFiles[trackId].seek(0);
  }

size_t Storage::readTrack(int trackId, void* buffer, size_t bytes) {
    return musicFiles[trackId].read(buffer, bytes);
  }

bool Storage::trackFileIsOpen(int trackId) const {
    return musicFiles[trackId].isOpen();
  }

bool Storage::loadToBuffer(String filename, uint16_t* buffer) {
    FsFile file = sdfat.open(filename.c_str(), O_READ);
    if (!file) return false; // 失敗

    // 指定された場所にデータを流し込む
    file.read((void*)buffer, 64 * 64 * 2);
    file.close();
    return true;
  }

FsFile Storage::openFile(const char* filename, uint8_t flags) {
    return sdfat.open(filename, flags);
  }

bool Storage::existsFile(const char* filename) {
    return sdfat.exists(filename);
  }
