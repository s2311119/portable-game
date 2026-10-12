#include "InnStaySystem.h"

#include "Inventory.h"
#include "Player.h"
#include "Party.h"

void InnStaySystem::stay(int cost, Inventory& inventory, Player& hero,
                         Party& party) {
  inventory.money -= cost;

  hero.state().revive(100);
  hero.state().heal(9999);
  hero.state().restoreTP(9999);
  for (size_t i = 0; i < party.memberCount(); ++i) {
    CharacterState& member = party.memberState(i);
    member.revive(100);
    member.heal(9999);
    member.restoreTP(9999);
  }
}
