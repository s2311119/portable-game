#pragma once

class CharacterState;

class EvolutionSystem {
public:
  static int nextLevelEvolutionId(const CharacterState& character);
  static void applyIdentity(CharacterState& character, int nextId);
};
