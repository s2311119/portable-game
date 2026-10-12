#include "ImageManager.h"

#include "Sd.h"
#include <cstring>

ImageManager::ImageManager() {
  // 起動時に確保しきる！これでプレイ中の new / delete はゼロになる
  for (int i = 0; i < 8; i++) {
    pool[i] = new uint16_t[64 * 64];
    // 黒で塗りつぶしておく
    memset(pool[i], 0, 64 * 64 * 2);
  }
}

// 指定したスロット(0-7)に画像を読み込み、そのポインタを返す
uint16_t* ImageManager::load(int slot, String filename, Sd& sd) {
  if (slot < 0 || slot >= 8) return nullptr;

  // SDカードから、確保済みのメモリプールへ上書きロード
  // (Sdクラスに追加した loadToBuffer を使う)
  if (sd.loadToBuffer(filename, pool[slot])) {
      return pool[slot]; // 成功したらポインタを返す
  } else {
      // 失敗したら黒く塗って返す（エラー落ち防止）
      memset(pool[slot], 0, 64 * 64 * 2);
      return pool[slot];
  }
}

// 指定スロットの画像を取得
uint16_t* ImageManager::get(int slot) {
    if (slot < 0 || slot >= 8) return nullptr;
    return pool[slot];
}

// Read-only battle views. Active-slot metadata is added with BattleImageRequest.
const uint16_t* ImageManager::enemyImage(int enemyIndex) const {
    if (enemyIndex < 0 || enemyIndex >= 4) return nullptr;
    return pool[enemyIndex];
}

const uint16_t* ImageManager::playerImage() const {
    return pool[4];
}

const uint16_t* ImageManager::allyImage(int allyIndex) const {
    if (allyIndex < 0 || allyIndex >= 3) return nullptr;
    return pool[5 + allyIndex];
}

// 全消去（実際はメモリを解放せず、黒く塗るだけ）
// これが「擬似的なガベージコレクション」になります
void ImageManager::clearAll() {
    for (int i = 0; i < 8; i++) {
        memset(pool[i], 0, 64 * 64 * 2);
    }
}
