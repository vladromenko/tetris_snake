#ifndef BRICK_GAME_API_H_
#define BRICK_GAME_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Action
} UserAction_t;

typedef struct {
  int **field;
  int **next;
  int score;
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo_t;

void userInput(UserAction_t action, bool hold);
GameInfo_t updateCurrentState(void);
int isGameOver(void);
void freeGameInfo(GameInfo_t *g);

#ifdef __cplusplus
}
#endif

#endif
