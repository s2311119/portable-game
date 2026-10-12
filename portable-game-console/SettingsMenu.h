#pragma once

class Settings;

enum class SettingsMenuChange {
  NONE,
  BGM_CHANGED,
  SE_CHANGED,
  VIBRATION_CHANGED
};

enum class SettingsMenuResult {
  NONE,
  BACK
};

class SettingsMenu {
public:
  void open();
  void moveUp();
  void moveDown();
  SettingsMenuChange change(Settings& settings, bool rightPressed) const;
  SettingsMenuResult confirm() const;
  SettingsMenuResult cancel() const;

  int cursor() const { return cursor_; }

private:
  int cursor_ = 0;
};
