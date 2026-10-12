#pragma once

class StorageMenu;
class Party;
class AutomatonStorage;
class Graphic;

class StorageRenderer {
public:
  void drawLine(const StorageMenu& menu, const Party& party,
                const AutomatonStorage& storage, Graphic& graphic,
                int drawY) const;
};
