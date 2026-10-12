#pragma once

class Vibration {
public:
  // ★ 振動モーターを繋ぐピン (回路に合わせて変更してください)
  static const int VIB_PIN = 7; 
  unsigned long vibrationEndTime = 0;
  bool _enabled = true;

  void init();

  // msミリ秒だけ振動させる
  void trigger(int ms);

  void setEnabled(bool enabled);
  // loop内で毎回呼ぶ
  void update();
};
