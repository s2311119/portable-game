#pragma once

class Inventory;
class Player;
class Party;

class InnStaySystem {
public:
  static void stay(int cost, Inventory& inventory, Player& hero,
                   Party& party);
};
