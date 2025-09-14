#include <cstdlib>

#include "brick_game_api.h"
#include "snake.h"

namespace s21 {

void SnakeHandleInput(UserAction_t action, bool hold);

namespace {

/**
 * @brief Выделяет и заполняет нулями матрицу int размером rows×cols.
 *
 * @param rows     Количество строк.
 * @param cols     Количество столбцов.
 * @param[out] out_rows  Адрес переменной типа int**
 * @details
 *   Матрица представлена как «массив указателей на строки»:
 *     - верхний уровень: int** (указатели на строки),
 *     - каждая строка:   int*  (массив из cols целых).
 *   В C++ std::malloc возвращает void*, поэтому применяется static_cast
 *   к нужному типу указателя (int** и int*), чтобы указать компилятору,
 *   каким типом пользоваться этой памятью.
 */

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

/**
 * @brief Инициализирует единственный экземпляр «Змейки» один раз.
 * @details
 *  - Не принимает параметров и не возвращает значения.
 *  - Внутри хранит статический флаг initialized.
 *  - При первом вызове выполняет GlobalSnake().Init(10, 20) —
 *    задаёт геометрию поля, сбрасывает состояние (тело, еда,
 *    флаги, счётчики), подготавливает игру к шагам FSM.
 *  - При последующих вызовах сразу выходит, не выполняя повторной
 *    инициализации.
 * @note Если убрать вызовы EnsureInit() из C‑API
 * (userInput/updateCurrentState), UI может обращаться к игре до подготовки
 * рантайма, что приведёт к неверному состоянию (например, пустое
 * тело/отсутствующая еда).
 */
void EnsureInit() {
  static int initialized = 0;
  if (initialized == 0) {
    GlobalSnake().Init(10, 20);
    initialized = 1;
  }
}

/**
 * @brief Создаёт и возвращает матрицу поля размером h×w (тип int**).
 *
 * @param h  Количество строк (высота).
 * @param w  Количество столбцов (ширина).
 * @return   Верхний указатель на массив указателей на строки (int**);
 *           nullptr, если выделение памяти не удалось.
 *
 * @details  Обёртка над AllocateMatrix(h, w, &rows): выделяет память под
 *           матрицу и инициализирует её нулями внутри AllocateMatrix, затем
 *           возвращает полученный указатель.
 *
 */

static int** BuildFieldMatrix(int h, int w) {
  int** rows = nullptr;
  AllocateMatrix(h, w, &rows);
  return rows;
}

/**
 * @brief Нанести тело змейки и еду на матрицу поля для UI.
 *
 * @param field матрица поля h×w (int**), где 0 — пусто, 1 — сегмент тела, 2 —
 * еда
 * @param h     число строк матрицы (высота поля)
 * @param w     число столбцов матрицы (ширина поля)
 *
 * @details
 *  - Функция не выделяет и не очищает память: ожидается, что `field` уже
 *    создана и заполнена нулями (например, через BuildFieldMatrix()).
 *  - Сначала пробегает по deque сегментов тела (`GlobalSnake().Body()`)
 *    и для каждой клетки, попадающей в границы [0..h]×[0..w], ставит 1.
 *    Сегменты вне поля безопасно игнорируются.
 *  - Затем наносит еду: если координаты в пределах
 *    поля — ставит 2.
 */
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
}  // namespace

}  // namespace s21

extern "C" {

void userInput(UserAction_t action, bool hold) {
  s21::EnsureInit();
  s21::SnakeHandleInput(action, hold);
}

GameInfo_t updateCurrentState(void) {
  s21::EnsureInit();
  s21::GlobalSnake().Step();

  int h = s21::GlobalSnake().Height();
  int w = s21::GlobalSnake().Width();
  int** field_rows = s21::BuildFieldMatrix(h, w);
  s21::RenderBodyAndFood(field_rows, h, w);
  int** next_rows = s21::BuildEmptyNextPreview();

  GameInfo_t g;
  g.field = field_rows;
  g.next = next_rows;
  g.score = s21::GlobalSnake().Score();
  g.high_score = s21::GlobalSnake().HighScore();
  g.level = s21::GlobalSnake().Level();
  g.speed = s21::GlobalSnake().SpeedMs();
  g.pause = s21::GlobalSnake().Paused() ? 1 : 0;
  return g;
}

void freeGameInfo(GameInfo_t* g) {
  if (g != nullptr) {
    s21::FreeMatrix(s21::GlobalSnake().Height(), &g->field);
    s21::FreeMatrix(4, &g->next);
  }
}

int isGameOver(void) {
  s21::EnsureInit();
  int result = 0;
  if (s21::GlobalSnake().GameOver()) {
    result = 1;
  }
  return result;
}

int isWin(void) {
  int result = 0;
  return result;
}

int t_take_terminate(void) {
  s21::EnsureInit();
  static int latched = 0;
  int result = 0;

  if (s21::GlobalSnake().GameOver() || s21::GlobalSnake().TakeTerminateOnce()) {
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
