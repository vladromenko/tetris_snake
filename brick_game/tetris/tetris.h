#ifndef TETRIS_H_
#define TETRIS_H_

#include "brick_game_api.h"

typedef enum {
  STATE_START = 0,
  STATE_SPAWN,
  STATE_INPUT,
  STATE_DROP,
  STATE_FIX,
  STATE_PAUSED,
  STATE_GAMEOVER
} TetrisState;

enum { T_ROWS = 20, T_COLS = 10 };

typedef struct {
  int m[4][4];
} Shape;

typedef struct {
  Shape shape;
  int x;
  int y;
} Active;

void t_init(void);
TetrisState t_get_state(void);
void t_set_state(TetrisState s);

int **t_field_rows(void);
int **t_next_rows(void);

int t_get_score(void);
int t_get_level(void);
int t_get_speed_ms(void);
int t_is_paused(void);
void t_set_paused(int v);
void t_input_reset(void);

int t_get_tick_limit(void);
void t_speed_inc(void);
void t_speed_dec(void);

int t_tick_ready(void);
void t_tick_reset(void);

int t_get_high_score(void);
void t_reset_for_new_game(void);

void handle_input(UserAction_t action, int hold);
int t_take_test(UserAction_t *out);
int t_take(UserAction_t *out);
int t_is_fast_drop(void);
int t_take_terminate(void);

void fsm_step(void);
void logic_start(void);
void logic_spawn(void);
void logic_input(void);
void logic_drop(void);
void logic_fix(void);
void logic_paused(void);
void logic_gameover(void);

void t_reload_high_score(void);

void t_clear_field(void);
void t_copy_board_to_field(void);
void t_render_active_to_field(void);
int t_can_move(int dx, int dy);
int t_try_move(int dx, int dy);
void t_rotate_cw(void);
int t_can_drop(void);
void t_drop_one(void);
void t_hard_drop(void);
void t_fix_to_board(void);
int t_spawn_new_piece(void);
void t_build_next_preview(void);
int t_clear_full_lines(void);

#endif
