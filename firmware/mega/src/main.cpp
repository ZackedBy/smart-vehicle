#include <Arduino.h>

#include "board.hpp"
#include "pilot.hpp"

Board board;
Pilot pilot{board};

void setup() {
  board.begin();
  pilot.begin();
}

void loop() {
  if (!board.rgb_ready()) {
    board.render();
    return;
  }
  pilot.tick();
}
