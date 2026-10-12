#pragma once

#include <Arduino.h>
#include <stdint.h>

class Sd;

struct BattleImageRequest {
  int enemyIds[4];
  int enemyCount;
  int playerCharacterId;
  int allyCharacterIds[3];
  int allyCount;
};

class ImageManager {
private:
  // 64x64画像 x 8枚分 のメモリを確保 (合計 64KB)
  // これが「メモリプール」です。ここ以外には画像を置きません。
  uint16_t* pool[8];
  bool activeSlots[8];
  Sd& sd;

  void load(int slot, const String& filename);

public:
  explicit ImageManager(Sd& sd);
  void prepareEnemyImages(const BattleImageRequest& request);
  void preparePartyImages(const BattleImageRequest& request);
  const uint16_t* enemyImage(int enemyIndex) const;
  const uint16_t* playerImage() const;
  const uint16_t* allyImage(int allyIndex) const;
};
