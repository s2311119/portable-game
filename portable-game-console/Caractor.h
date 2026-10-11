#pragma once

class Map;
class Status;

class Caractor {
public:
  enum Direction { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };
  Direction direction = DIR_DOWN;
  int X, oldX, Y, oldY;
  bool moved = false;
  int MOVE_SPEED = 8;
  Status* status;
  // Mapオブジェクトを受け取り、安全なスタート地点を探す
  Caractor(Map& map, int level, int who);
  
  void reset(Map& map);
  ~Caractor();
};
