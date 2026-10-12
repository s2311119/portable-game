#include "SettingsMenu.h"

#include "Settings.h"

void SettingsMenu::open() {
  cursor_ = 0;
}

void SettingsMenu::moveUp() {
  cursor_ = (cursor_ - 1 + 4) % 4;
}

void SettingsMenu::moveDown() {
  cursor_ = (cursor_ + 1) % 4;
}

SettingsMenuChange SettingsMenu::change(Settings& settings,
                                         bool rightPressed) const {
  int amount = rightPressed ? 1 : -1;
  if (cursor_ == 0) {
    settings.setBgmVolume(settings.getBgmVolume() + amount);
    return SettingsMenuChange::BGM_CHANGED;
  }
  if (cursor_ == 1) {
    settings.setSeVolume(settings.getSeVolume() + amount);
    return SettingsMenuChange::SE_CHANGED;
  }
  if (cursor_ == 2) {
    settings.setVibrationEnabled(!settings.isVibrationEnabled());
    return SettingsMenuChange::VIBRATION_CHANGED;
  }
  return SettingsMenuChange::NONE;
}

SettingsMenuResult SettingsMenu::confirm() const {
  return cursor_ == 3 ? SettingsMenuResult::BACK : SettingsMenuResult::NONE;
}

SettingsMenuResult SettingsMenu::cancel() const {
  return SettingsMenuResult::BACK;
}
