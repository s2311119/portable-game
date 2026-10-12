#include "Game.h"

void setup() {
  Game::boot();
}

void loop() {
  ctx->update();
}