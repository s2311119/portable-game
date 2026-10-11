#pragma once

class Settings {
public:
  int getBgmVolume() const { return bgmVolume; }
  int getSeVolume() const { return seVolume; }
  bool isVibrationEnabled() const { return vibration; }

  void setBgmVolume(int value) {
    if (value < 0) value = 0;
    if (value > 10) value = 10;
    bgmVolume = value;
  }

  void setSeVolume(int value) {
    if (value < 0) value = 0;
    if (value > 10) value = 10;
    seVolume = value;
  }

  void setVibrationEnabled(bool enabled) { vibration = enabled; }

private:
  int bgmVolume = 10; // 0〜10 (初期値5)
  int seVolume = 10;  // 0〜10 (初期値5)
  bool vibration = true; // ON/OFF
};
