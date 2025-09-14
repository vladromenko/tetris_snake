#include "tetris.h"

extern int t_is_fast_drop(void);
extern int t_take_terminate(void);
extern int t_take(UserAction_t *out);


void fsm_step(void) {
  TetrisState s = t_get_state();
  int terminated = 0;
  if (s != STATE_START) {
    if (t_take_terminate() != 0) {
      terminated = 1;
    }
  }

  if (terminated) {
    t_set_state(STATE_GAMEOVER);
  } else {
    if (s == STATE_START) {
      logic_start();
    } else if (s == STATE_SPAWN) {
      logic_spawn();
    } else if (s == STATE_INPUT) {
      logic_input();
    } else if (s == STATE_DROP) {
      logic_drop();
    } else if (s == STATE_FIX) {
      logic_fix();
    } else if (s == STATE_PAUSED) {
      logic_paused();
    } else if (s == STATE_GAMEOVER) {
      logic_gameover();
    }
  }
}


void logic_start(void) {
  t_input_reset();
  t_clear_field();
  t_tick_reset();
  t_reload_high_score();
  t_build_next_preview();
  t_set_state(STATE_SPAWN);
}


void logic_spawn(void) {
  int ok = t_spawn_new_piece();
  TetrisState next = STATE_GAMEOVER;
  if (ok != 0) {
    t_build_next_preview();
    next = STATE_INPUT;
  }
  t_set_state(next);
}


void logic_input(void) {
  TetrisState next = STATE_DROP;
  UserAction_t a;
  if (t_take(&a) != 0) {
    if (a == Terminate) {
      next = STATE_GAMEOVER;
    } else if (a == Left) {
      (void)t_try_move(-1, 0);
    } else if (a == Right) {
      (void)t_try_move(+1, 0);
    } else if (a == Up) {
      t_rotate_cw();
    } else if (a == Action) {
      t_hard_drop();
      next = STATE_FIX;
    } else if (a == Down) {
      if (t_can_drop() != 0) {
        t_drop_one();
        next = STATE_INPUT;
      } else {
        next = STATE_FIX;
      }
    } else if (a == Pause) {
      t_set_paused(1);
      next = STATE_PAUSED;
    }
  }
  t_set_state(next);
}


void logic_drop(void) {
  TetrisState next = STATE_INPUT;
  if (t_is_fast_drop() != 0) {
    if (t_can_drop() != 0) {
      t_drop_one();
      next = STATE_INPUT;
    } else {
      next = STATE_FIX;
    }
  } else if (t_tick_ready() != 0) {
    if (t_can_drop() != 0) {
      t_drop_one();
      next = STATE_INPUT;
    } else {
      next = STATE_FIX;
    }
  }
  t_set_state(next);
}


void logic_fix(void) {
  t_fix_to_board();
  if (t_get_state() != STATE_GAMEOVER) {
    t_clear_full_lines();
    t_tick_reset();
    t_set_state(STATE_SPAWN);
  }
}


void logic_paused(void) {
  TetrisState next = STATE_PAUSED;
  UserAction_t a;
  if (t_take(&a) != 0) {
    if (a == Terminate) {
      next = STATE_GAMEOVER;
    } else if (a == Pause) {
      t_set_paused(0);
      next = STATE_INPUT;
    }
  }
  t_set_state(next);
}


void logic_gameover(void) {
  TetrisState next = STATE_GAMEOVER;
  UserAction_t a;
  if (t_take(&a) != 0) {
    if (a == Terminate) {
      next = STATE_GAMEOVER;
    } else if (a == Start) {
      next = STATE_START;
    }
  }
  t_set_state(next);
}
