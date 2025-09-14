#include "tetris.h"


typedef struct {
  int hasAction;
  UserAction_t currentAction;
  int isHoldDown;
  int terminateRequested;
} TInputState;


static TInputState *t_input_state(void) {
  static TInputState st = {0};
  return &st;
}
#define IS (*t_input_state())


void handle_input(UserAction_t action, int hold) {
  if (action == Down) {
    IS.isHoldDown = (hold != 0) ? 1 : 0;
  }

  if (action == Terminate) {
    IS.terminateRequested = 1;
  }

  IS.currentAction = action;
  IS.hasAction = 1;
}


int t_take(UserAction_t *outAction) {
  int result = 0;

  if (outAction != 0) {
    if (IS.hasAction != 0) {
      *outAction = IS.currentAction;
      IS.hasAction = 0;
      result = 1;
    }
  }

  return result;
}


int t_take_test(UserAction_t *out) { return t_take(out); }


int t_is_fast_drop(void) {
  int result = 0;
  if (IS.isHoldDown != 0) {
    result = 1;
  }
  return result;
}


int t_take_terminate(void) {
  int result = 0;
  if (IS.terminateRequested != 0) {
    IS.terminateRequested = 0;
    if (IS.hasAction != 0 && IS.currentAction == Terminate) {
      IS.hasAction = 0;
    }
    result = 1;
  }
  return result;
}


void t_input_reset(void) {
  IS.hasAction = 0;
  IS.currentAction = Start;
  IS.isHoldDown = 0;
  IS.terminateRequested = 0;
}
