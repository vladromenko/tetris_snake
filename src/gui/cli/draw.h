#ifndef DRAW_H
#define DRAW_H

#include "brick_game_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void interface_run(void);
void console_loop(void);
int step_and_draw_once(void);
void interface_draw(const GameInfo_t *g);
void draw_game_over_banner_over_field(int top, int left, int rows, int cols,
                                      int cellw, int cellh);
void draw_hud_classic(int top, int left, const GameInfo_t *g);
void sleep_ms(int ms);
void draw_border_classic(int top, int left, int rows, int cols, int cellw,
                         int cellh);
void draw_matrix_classic(int top, int left, int **grid, int rows, int cols,
                         int cellw, int cellh);
void map_key_to_action(int ch, UserAction_t *act, int *hold);
int read_last_keypress(void);
void process_last_key(int last, int *action_down, int *quit_overlay);
int t_take_terminate(void);

#ifdef __cplusplus
}
#endif

#endif  // DRAW_H
