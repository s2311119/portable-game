#pragma once

class SettingsMenu;
class Settings;
class Graphic;

class SettingsRenderer {
public:
  void drawLine(const SettingsMenu& menu, const Settings& settings,
                Graphic& graphic, int drawY) const;
};
