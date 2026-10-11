#include "SaveManager.h"
#include "Sd.h"
#include "MUSIC.h"
#include "Map.h"
#include "WorldProgress.h"
#include "Player.h"
#include "Party.h"
#include "AutomatonStorage.h"
#include "Inventory.h"
#include "Automaton.h"
#include <utility>

  // ★ セーブ実行
  bool SaveManager::saveGame(
      Sd& sd,
      MUSIC& music,
      Map& map,
      WorldProgress& progress,
      Player& hero,
      Party& party,
      AutomatonStorage& storage,
      Inventory& inventory
  ) {
    music.paused = true;
    critical_section_enter_blocking(&sdLock);

    FsFile file =
        sd.sdfat.open(
            SAVE_FILENAME,
            O_WRITE | O_CREAT | O_TRUNC
        );

    if (!file) {
      critical_section_exit(&sdLock);
      music.paused = false;
      return false;
    }

    // -------------------------------------------------
    // セーブ形式
    // -------------------------------------------------
    file.println(SAVE_MAGIC);

    // -------------------------------------------------
    // 1. 現在のマップ状態
    // -------------------------------------------------
    file.println(progress.floor());
    file.println((int)map.getType());

    file.println(map.getMAP_WIDTH());
    file.println(map.getMAP_HEIGHT());

    // マップ上の未回収オートマタの種類
    file.println(map.getAutomatonWhoId());

    // 主人公の現在位置・向き
    file.println(hero.X);
    file.println(hero.Y);
    file.println((int)hero.direction);

    // マップそのもの
    file.println(
        map.getMAP_WIDTH() *
        map.getMAP_HEIGHT()
    );

    for (int y = 0; y < map.getMAP_HEIGHT(); y++) {
      for (int x = 0; x < map.getMAP_WIDTH(); x++) {
        file.println((int)map.getTile(x, y));
      }
    }

    // -------------------------------------------------
    // 2. 主人公
    // -------------------------------------------------
    hero.state().saveTo(file);

    // -------------------------------------------------
    // 3. インベントリ
    // -------------------------------------------------
    inventory.saveTo(file);

    // -------------------------------------------------
    // 4. パーティー
    // -------------------------------------------------
    file.println(party.memberCount());

    for (size_t i = 0; i < party.memberCount(); ++i) {
      party.member(i).state().saveTo(file);
    }

    // -------------------------------------------------
    // 5. 預かり所
    // -------------------------------------------------
    file.println(storage.memberCount());

    for (size_t i = 0; i < storage.memberCount(); ++i) {
      storage.member(i).state().saveTo(file);
    }

    // -------------------------------------------------
    // 6. ボス撃破状態
    // -------------------------------------------------
    for (int i = 0; i < 5; i++) {
      file.println(progress.bosses().isDefeated(i));
    }

    file.close();

    critical_section_exit(&sdLock);
    music.paused = false;

    return true;
  }

  // ★ ロード実行
  bool SaveManager::loadGame(
      Sd& sd,
      MUSIC& music,
      Map& map,
      WorldProgress& progress,
      Player& hero,
      Party& party,
      AutomatonStorage& storage,
      Inventory& inventory
  ) {
    FsFile file =
        sd.sdfat.open(
            SAVE_FILENAME,
            O_READ
        );

    if (!file) {
      return false;
    }

    // ロード失敗時の共通終了処理
    auto failLoad = [&]() -> bool {
      file.close();
      return false;
    };

    // -------------------------------------------------
    // セーブ形式判定
    // -------------------------------------------------
    String firstLine =
        file.readStringUntil('\n');

    firstLine.trim();

    bool isNewFormat =
        (firstLine == SAVE_MAGIC);

    Map::MapType loadedType =
        Map::TYPE_DUNGEON;

    int savedHeroX = 0;
    int savedHeroY = 0;
    int savedDirection =
        (int)Player::DIR_DOWN;

    if (isNewFormat) {
      // -------------------------------------------------
      // PGSAVE2
      // -------------------------------------------------
      progress.restoreFloor(
          file.readStringUntil('\n').toInt()
      );

      int typeVal =
          file.readStringUntil('\n').toInt();

      if (typeVal < (int)Map::TYPE_MAZE ||
          typeVal > (int)Map::TYPE_BOSS_ROOM)
      {
        return failLoad();
      }

      loadedType =
          static_cast<Map::MapType>(
              typeVal
          );

      int savedWidth =
          file.readStringUntil('\n').toInt();

      int savedHeight =
          file.readStringUntil('\n').toInt();

      // 異常なセーブデータで巨大なvectorを確保しないための保護
      if (savedWidth <= 0 ||
          savedHeight <= 0 ||
          savedWidth > 64 ||
          savedHeight > 64)
      {
        return failLoad();
      }

      map.restoreAutomatonWhoId(
          file.readStringUntil('\n').toInt()
      );

      savedHeroX =
          file.readStringUntil('\n').toInt();

      savedHeroY =
          file.readStringUntil('\n').toInt();

      savedDirection =
          file.readStringUntil('\n').toInt();

      if (savedDirection < (int)Player::DIR_DOWN ||
          savedDirection > (int)Player::DIR_RIGHT)
      {
        return failLoad();
      }

      int mapDataCount =
          file.readStringUntil('\n').toInt();

      if (mapDataCount !=
          savedWidth * savedHeight)
      {
        return failLoad();
      }

      if (savedHeroX < 0 ||
          savedHeroY < 0 ||
          savedHeroX >= savedWidth * Map::TILE_SIZE ||
          savedHeroY >= savedHeight * Map::TILE_SIZE)
      {
        return failLoad();
      }

      map.beginRestoreFromSave(
          loadedType,
          savedWidth,
          savedHeight
      );

      for (int i = 0;
          i < mapDataCount;
          i++)
      {
        int tile =
            file.readStringUntil('\n').toInt();

        if (tile < 0 || tile > 255) {
          return failLoad();
        }

        int x = i % savedWidth;
        int y = i / savedWidth;

        map.setTile(
            x,
            y,
            (uint8_t)tile
        );
      }
    }
    else {
      // -------------------------------------------------
      // 旧形式
      //
      // 旧セーブにはマップ本体がないため、
      // 従来どおり階層とMapTypeだけ読み、
      // 最後にマップを再生成する。
      // -------------------------------------------------
      progress.restoreFloor(
          firstLine.toInt()
      );

      int typeVal =
          file.readStringUntil('\n').toInt();

      if (typeVal < (int)Map::TYPE_MAZE ||
          typeVal > (int)Map::TYPE_BOSS_ROOM)
      {
        return failLoad();
      }

      loadedType =
          static_cast<Map::MapType>(
              typeVal
          );
    }

    // -------------------------------------------------
    // 主人公
    // -------------------------------------------------
    hero.state().loadFrom(file);

    // -------------------------------------------------
    // インベントリ
    // -------------------------------------------------
    inventory.loadFrom(file);

    // -------------------------------------------------
    // パーティー
    // -------------------------------------------------
    party.clearMembers();

    int memberCount =
        file.readStringUntil('\n').toInt();

    for (int i = 0;
        i < memberCount;
        i++)
    {
      Automaton ally(1, 0);
      ally.state().loadFrom(file);
      party.appendRestoredMember(std::move(ally));
    }

    // -------------------------------------------------
    // 預かり所
    // -------------------------------------------------
    storage.clearForRestore();

    int storageCount =
        file.readStringUntil('\n').toInt();

    for (int i = 0;
        i < storageCount;
        i++)
    {
      Automaton ally(1, 0);
      ally.state().loadFrom(file);
      storage.appendRestoredMember(std::move(ally));
    }

    // -------------------------------------------------
    // ボス撃破状態
    // -------------------------------------------------
    for (int i = 0; i < 5; i++) {
      progress.restoreBossDefeated(i,
          (
            file.readStringUntil('\n').toInt()
            != 0
          ));
    }

    // -------------------------------------------------
    // マップ・主人公位置の復元
    // -------------------------------------------------
    if (isNewFormat) {
      // 新形式は保存したマップをそのまま使用する。
      // regenerate() は絶対に呼ばない。

      hero.X =
          savedHeroX;

      hero.Y =
          savedHeroY;

      hero.oldX =
          savedHeroX;

      hero.oldY =
          savedHeroY;

      hero.direction =
          (Player::Direction)
              savedDirection;

      hero.moved = false;
    }
    else {
      // 旧形式はマップ本体を持っていないので
      // 従来どおり再生成する。

      Map::MapType finalType =
          loadedType;

      // ボス階では撃破状態を優先
      if (progress.floor() >= 10 &&
          progress.floor() <= 50 &&
          progress.floor() % 10 == 0)
      {
        int bossIndex =
            (progress.floor() / 10) - 1;

        if (bossIndex >= 0 &&
            bossIndex < 5)
        {
          if (progress.bosses().isDefeated(bossIndex)) {
            finalType =
                Map::TYPE_TOWN;
          }
          else {
            finalType =
                Map::TYPE_BOSS_ROOM;
          }
        }
      }

      map.regenerate(
          20,
          20,
          finalType,
          progress.floor()
      );

      hero.reset(map);
    }

    file.close();

    return true;
  }

  // ファイルが存在するかチェック
  bool SaveManager::hasSaveFile(Sd& sd) {
     return sd.sdfat.exists(SAVE_FILENAME);
  }
