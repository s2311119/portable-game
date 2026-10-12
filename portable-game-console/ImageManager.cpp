#include "ImageManager.h"

#include "Sd.h"
#include <cstring>

ImageManager::ImageManager(Sd& sd) : sd(sd) {
  // 起動時に固定8枚分を確保し、戦闘中の確保・解放は行わない。
  for (int i = 0; i < 8; ++i) {
    pool[i] = new uint16_t[64 * 64];
    memset(pool[i], 0, 64 * 64 * 2);
    slotInfo[i] = {ImageKind::NONE, -1, false, false};
  }
}

void ImageManager::load(int slot, ImageKind kind, int id) {
  ImageSlotInfo& info = slotInfo[slot];
  info.active = true;
  if (info.loaded && info.kind == kind && info.id == id) return;

  const String filename =
      (kind == ImageKind::ENEMY ? "/e_" : "/a_") + String(id) + ".bin";
  if (sd.loadToBuffer(filename, pool[slot])) {
    info.kind = kind;
    info.id = id;
    info.loaded = true;
  } else {
    // 読み込み失敗時も従来どおり黒画像で描画を続ける。
    memset(pool[slot], 0, 64 * 64 * 2);
    info.kind = ImageKind::NONE;
    info.id = -1;
    info.loaded = false;
  }
}

void ImageManager::prepareEnemyImages(const BattleImageRequest& request) {
  critical_section_enter_blocking(&sdLock);
  for (int i = 0; i < 4; ++i) {
    slotInfo[i].active = i < request.enemyCount;
    if (slotInfo[i].active) {
      load(i, ImageKind::ENEMY, request.enemyIds[i]);
    }
  }
  critical_section_exit(&sdLock);
}

void ImageManager::preparePartyImages(const BattleImageRequest& request) {
  critical_section_enter_blocking(&sdLock);
  load(4, ImageKind::CHARACTER, request.playerCharacterId);
  for (int i = 0; i < 3; ++i) {
    slotInfo[5 + i].active = i < request.allyCount;
    if (slotInfo[5 + i].active) {
      load(5 + i, ImageKind::CHARACTER, request.allyCharacterIds[i]);
    }
  }
  critical_section_exit(&sdLock);
}

const uint16_t* ImageManager::enemyImage(int enemyIndex) const {
  if (enemyIndex < 0 || enemyIndex >= 4 || !slotInfo[enemyIndex].active) return nullptr;
  return pool[enemyIndex];
}

const uint16_t* ImageManager::playerImage() const {
  return slotInfo[4].active ? pool[4] : nullptr;
}

const uint16_t* ImageManager::allyImage(int allyIndex) const {
  if (allyIndex < 0 || allyIndex >= 3 || !slotInfo[5 + allyIndex].active) return nullptr;
  return pool[5 + allyIndex];
}

void ImageManager::invalidateAll() {
  for (int i = 0; i < 8; ++i) {
    slotInfo[i] = {ImageKind::NONE, -1, false, false};
  }
}
