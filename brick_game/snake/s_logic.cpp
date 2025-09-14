#include "snake.h"

namespace snake {

enum SnakeState {
  STATE_START = 0,
  STATE_INPUT,
  STATE_DROP,
  STATE_FIX,
  STATE_PAUSED,
  STATE_GAMEOVER
};

static SnakeState& SnakeStateRef() {
  static SnakeState state = STATE_START;
  return state;
}

void SnakeGame::Step() {
  SnakeState& st = SnakeStateRef();
  if (st == STATE_GAMEOVER && !game_over_) st = STATE_START;
  if (st == STATE_PAUSED && !paused_) st = STATE_INPUT;

  SnakeState next = st;
  switch (st) {
    case STATE_START:
      FSM_StepStart();
      next = STATE_INPUT;
      break;
    case STATE_INPUT:
      FSM_StepInput();
      if (paused_) {
        next = STATE_PAUSED;
      } else if (game_over_ || TakeTerminateOnce()) {
        next = STATE_GAMEOVER;
      } else {
        next = STATE_DROP;
      }
      break;
    case STATE_DROP:
      if (FSM_StepDrop()) {
        next = STATE_FIX;
      } else {
        next = STATE_DROP;
      }
      break;
    case STATE_FIX:
      if (FSM_StepFix()) {
        next = STATE_INPUT;
      } else {
        next = STATE_GAMEOVER;
      }
      break;
    case STATE_PAUSED:
      FSM_StepPaused();
      next = STATE_PAUSED;
      break;
    case STATE_GAMEOVER:
      FSM_StepGameOver();
      next = STATE_GAMEOVER;
      break;
  }
  st = next;
}

}  // namespace snake
