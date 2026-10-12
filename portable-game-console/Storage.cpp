#include "Storage.h"

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
