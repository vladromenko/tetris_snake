#include <cstdlib>

#include "brick_game_api.h"
#include "snake.h"

namespace snake {

void SnakeHandleInput(UserAction_t action, bool hold);

namespace {

void AllocateMatrix(int rows, int cols, int*** out_rows) {
  int** rows_ptr = static_cast<int**>(std::malloc(sizeof(int*) * rows));
  int r = 0;
  while (r < rows) {
    rows_ptr[r] = static_cast<int*>(std::malloc(sizeof(int) * cols));
    int c = 0;
    while (c < cols) {
      rows_ptr[r][c] = 0;
      c = c + 1;
    }
    r = r + 1;
  }
  *out_rows = rows_ptr;
}

void FreeMatrix(int rows, int*** rows_ptr) {
  if (rows_ptr && *rows_ptr) {
    int r = 0;
    while (r < rows) {
      if ((*rows_ptr)[r]) std::free((*rows_ptr)[r]);
      r = r + 1;
    }
    std::free(*rows_ptr);
    *rows_ptr = nullptr;
  }
}


void EnsureInit() {
  static int initialized = 0;
  if (initialized == 0) {
    GlobalSnake().Init(10, 20);
    initialized = 1;
  }
}



static int** BuildFieldMatrix(int h, int w) {
  int** rows = nullptr;
  AllocateMatrix(h, w, &rows);
  return rows;
}


static void RenderBodyAndFood(int** field, int h, int w) {
  const std::deque<Point>& body = GlobalSnake().Body();
  int i = 0;
  while (i < static_cast<int>(body.size())) {
    int cx = body[i].x;
    int cy = body[i].y;
    if (cy >= 0 && cy < h && cx >= 0 && cx < w) {
      field[cy][cx] = 1;
    }
    i = i + 1;
  }
  Point food = GlobalSnake().Food();
  if (food.y >= 0 && food.y < h && food.x >= 0 && food.x < w) {
    field[food.y][food.x] = 2;
  }
}

static int** BuildEmptyNextPreview() {
  int** next = nullptr;
  AllocateMatrix(4, 4, &next);
  return next;
}
}

}  // namespace snake

extern "C" {

void userInput(UserAction_t action, bool hold) {
  snake::EnsureInit();
  snake::SnakeHandleInput(action, hold);
}

GameInfo_t updateCurrentState(void) {
  snake::EnsureInit();
  snake::GlobalSnake().Step();

  int h = snake::GlobalSnake().Height();
  int w = snake::GlobalSnake().Width();
  int** field_rows = snake::BuildFieldMatrix(h, w);
  snake::RenderBodyAndFood(field_rows, h, w);
  int** next_rows = snake::BuildEmptyNextPreview();

  GameInfo_t g;
  g.field = field_rows;
  g.next = next_rows;
  g.score = snake::GlobalSnake().Score();
  g.high_score = snake::GlobalSnake().HighScore();
  g.level = snake::GlobalSnake().Level();
  g.speed = snake::GlobalSnake().SpeedMs();
  g.pause = snake::GlobalSnake().Paused() ? 1 : 0;
  return g;
}

void freeGameInfo(GameInfo_t* g) {
  if (g != nullptr) {
    snake::FreeMatrix(snake::GlobalSnake().Height(), &g->field);
    snake::FreeMatrix(4, &g->next);
  }
}

int isGameOver(void) {
  snake::EnsureInit();
  int result = 0;
  if (snake::GlobalSnake().GameOver()) {
    result = 1;
  }
  return result;
}

int isWin(void) {
  int result = 0;
  return result;
}

int t_take_terminate(void) {
  snake::EnsureInit();
  static int latched = 0;
  int result = 0;

  if (snake::GlobalSnake().GameOver() || snake::GlobalSnake().TakeTerminateOnce()) {
    if (latched == 0) {
      result = 1;
      latched = 1;
    } else {
      result = 0;
    }
  } else {
    latched = 0;
  }

  return result;
}
}
