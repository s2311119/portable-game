#pragma once

#include <Arduino.h>
#include <stdint.h>

class Storage;

struct BattleImageRequest {
  int enemyIds[4];
  int enemyCount;
  int playerCharacterId;
  int allyCharacterIds[3];
  int allyCount;
};

class ImageManager {
private:
  enum class ImageKind {
    NONE,
    ENEMY,
    CHARACTER
  };

  struct ImageSlotInfo {
    ImageKind kind;
    int id;
    bool loaded;
    bool active;
  };

  // 64x64画像 x 8枚分 のメモリを確保 (合計 64KB)
  // これが「メモリプール」です。ここ以外には画像を置きません。
  uint16_t* pool[8];
  ImageSlotInfo slotInfo[8];
  Storage& sd;

  void load(int slot, ImageKind kind, int id);

public:
  explicit ImageManager(Storage& sd);
  void prepareEnemyImages(const BattleImageRequest& request);
  void preparePartyImages(const BattleImageRequest& request);
  const uint16_t* enemyImage(int enemyIndex) const;
  const uint16_t* playerImage() const;
  const uint16_t* allyImage(int allyIndex) const;
  void invalidateAll();
};
