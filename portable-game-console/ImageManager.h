#pragma once

#include <Arduino.h>
#include <stdint.h>

class Sd;

class ImageManager {
private:
  // 64x64画像 x 8枚分 のメモリを確保 (合計 64KB)
  // これが「メモリプール」です。ここ以外には画像を置きません。
  uint16_t* pool[8];

public:
  ImageManager();
  uint16_t* load(int slot, String filename, Sd& sd);
  uint16_t* get(int slot);
  const uint16_t* enemyImage(int enemyIndex) const;
  const uint16_t* playerImage() const;
  const uint16_t* allyImage(int allyIndex) const;
  void clearAll();
};
