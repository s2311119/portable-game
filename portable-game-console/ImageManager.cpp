#include "ImageManager.h"

#include "Sd.h"
#include <cstring>

ImageManager::ImageManager(Sd& sd) : sd(sd) {
  // 起動時に固定8枚分を確保し、戦闘中の確保・解放は行わない。
  for (int i = 0; i < 8; ++i) {
    pool[i] = new uint16_t[64 * 64];
    memset(pool[i], 0, 64 * 64 * 2);
    activeSlots[i] = false;
  }
}

void ImageManager::load(int slot, const String& filename) {
  if (!sd.loadToBuffer(filename, pool[slot])) {
    // 読み込み失敗時も従来どおり黒画像で描画を続ける。
    memset(pool[slot], 0, 64 * 64 * 2);
  }
}

void ImageManager::prepareEnemyImages(const BattleImageRequest& request) {
  critical_section_enter_blocking(&sdLock);
  for (int i = 0; i < 4; ++i) {
    activeSlots[i] = i < request.enemyCount;
    if (activeSlots[i]) {
      load(i, "/e_" + String(request.enemyIds[i]) + ".bin");
    }
  }
  critical_section_exit(&sdLock);
}

void ImageManager::preparePartyImages(const BattleImageRequest& request) {
  critical_section_enter_blocking(&sdLock);
  activeSlots[4] = true;
  load(4, "/a_" + String(request.playerCharacterId) + ".bin");
  for (int i = 0; i < 3; ++i) {
    activeSlots[5 + i] = i < request.allyCount;
    if (activeSlots[5 + i]) {
      load(5 + i, "/a_" + String(request.allyCharacterIds[i]) + ".bin");
    }
  }
  critical_section_exit(&sdLock);
}

const uint16_t* ImageManager::enemyImage(int enemyIndex) const {
  if (enemyIndex < 0 || enemyIndex >= 4 || !activeSlots[enemyIndex]) return nullptr;
  return pool[enemyIndex];
}

const uint16_t* ImageManager::playerImage() const {
  return activeSlots[4] ? pool[4] : nullptr;
}

const uint16_t* ImageManager::allyImage(int allyIndex) const {
  if (allyIndex < 0 || allyIndex >= 3 || !activeSlots[5 + allyIndex]) return nullptr;
  return pool[5 + allyIndex];
}
