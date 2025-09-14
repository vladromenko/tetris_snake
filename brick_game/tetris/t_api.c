#include "brick_game_api.h"
#include "tetris.h"


void userInput(UserAction_t action, bool hold) {
  t_init();
  int should_handle = 1;
  if (action == Action && !hold) {
    should_handle = 0;
  }
  if (should_handle) {
    handle_input(action, hold ? 1 : 0);
  }
}


GameInfo_t updateCurrentState(void) {
  t_init();

  fsm_step();

  t_clear_field();
  t_render_active_to_field();

  GameInfo_t g;
  g.field = t_field_rows();
  g.next = t_next_rows();
  g.score = t_get_score();
  g.high_score = t_get_high_score();
  g.level = t_get_level();
  g.speed = t_get_speed_ms();
  g.pause = t_is_paused();
  return g;
}


int isGameOver(void) {
  t_init();
  int result = 0;
  if (t_get_state() == STATE_GAMEOVER) {
    result = 1;
  }
  return result;
}

void freeGameInfo(GameInfo_t* g) { (void)g; }
