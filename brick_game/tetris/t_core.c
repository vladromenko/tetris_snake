#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tetris.h"

typedef struct {
  int board[T_ROWS][T_COLS];
  int field[T_ROWS][T_COLS];
  int *field_rows[T_ROWS];

  int next[4][4];
  int *next_rows[4];

  Active act;
  int next_id;
  int bag[7];
  int bag_index;

  int score;
  int high_score;
  int level;
  int lines_done;

  int tick_limit;
  int tick;

  TetrisState state;
  int paused;
} TCore;

static TCore *t_core_state(void) {
  static TCore s;
  return &s;
}
#define S (*t_core_state())

static int tick_limit_for_level(int level) {
  if (level < 1) level = 1;
  double base = 10.0;
  double factor = pow(1.5, (double)(level - 1));
  int lim = (int)lround(base / factor);
  if (lim < 2) lim = 2;
  return lim;
}

static void load_high_score(void);
static void save_high_score(void);
static void refill_bag(void);
static int blocked_at_cell(int shapeRow, int shapeCol, int dx, int dy);
static void blit_cell_to_field(int shapeRow, int shapeCol);
static void rotate_active_ccw(void);

static const Shape SHAPES[7] = {
    {{{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 1}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 1, 1}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 1, 0}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 1, 1}, {0, 0, 0, 0}}},
    {{{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 1}, {0, 0, 0, 0}}}};

static void bind_rows(void) {
  int rowIndex = 0;
  while (rowIndex < T_ROWS) {
    S.field_rows[rowIndex] = S.field[rowIndex];
    rowIndex++;
  }
  int previewRowIndex = 0;
  while (previewRowIndex < 4) {
    S.next_rows[previewRowIndex] = S.next[previewRowIndex];
    previewRowIndex++;
  }
}

/**
 * @brief Инициализировать единичное состояние тетриса.
 * @details
 *  Один раз за процесс обнуляет ядро, связывает указатели строк, выставляет
 *  стартовые значения (уровень, тики, состояние), перемешивает «мешок» фигур и
 *  подготавливает предпросмотр следующей фигуры. Также подгружает рекорд и
 *  гарантирует, что он не понизится.
 * @note Вызывается из C‑API перед каждым действием/снимком.
 */
void t_init(void) {
  static int inited = 0;

  if (inited == 0) {
    memset(&S, 0, sizeof(S));
    bind_rows();

    S.level = 1;
    S.lines_done = 0;

    S.tick_limit = 10;
    S.tick = 0;

    S.state = STATE_START;
    S.paused = 0;

    srand((unsigned)time(NULL));
    S.bag_index = 0;
    refill_bag();
    S.next_id = S.bag[S.bag_index++];
    S.high_score = 0;
    inited = 1;
    t_input_reset();
  }

  int prev_high = S.high_score;

  load_high_score();

  if (S.high_score < prev_high) {
    S.high_score = prev_high;
  }

  if (S.score > S.high_score) {
    S.high_score = S.score;
    save_high_score();
  }
}

/** @brief Текущий рекорд. */
int t_get_high_score(void) { return S.high_score; }

/**
 * @brief Перечитать рекорд с диска без понижения текущего значения.
 */
void t_reload_high_score(void) {
  int prev = S.high_score;
  load_high_score();
  if (S.high_score < prev) {
    S.high_score = prev;
  }
}

/** @brief Доступ к строкам матрицы поля для UI. */
int **t_field_rows(void) { return S.field_rows; }
/** @brief Доступ к строкам предпросмотра следующей фигуры (4×4). */
int **t_next_rows(void) { return S.next_rows; }

/** @brief Текущий счёт. */
int t_get_score(void) { return S.score; }
/** @brief Текущий уровень. */
int t_get_level(void) { return S.level; }

int t_get_speed_ms(void) {
  double base = 32.0;
  int lvl = S.level;
  if (lvl < 1) lvl = 1;
  double factor = pow(1.5, (double)(lvl - 1));
  int ms = (int)lround(base / factor);
  if (ms < 8) ms = 8;
  return ms;
}
/** @brief Признак паузы. */
int t_is_paused(void) { return S.paused; }
/** @brief Установить/снять паузу. */
void t_set_paused(int v) { S.paused = (v != 0); }

/** @brief Текущий лимит тиков до падения (для отладки/UI). */
int t_get_tick_limit(void) { return S.tick_limit; }

/** @brief Увеличить скорость (уменьшить лимит тиков), не ниже 2. */
void t_speed_inc(void) {
  if (S.tick_limit > 2) {
    S.tick_limit -= 1;
  }
}

/** @brief Уменьшить скорость (увеличить лимит тиков), верхняя граница 60. */
void t_speed_dec(void) {
  if (S.tick_limit < 60) {
    S.tick_limit += 1;
  }
}

/**
 * @brief Проверить готовность тикового шага падения.
 * @details
 *  Синхронизирует tick_limit с уровнем (через tick_limit_for_level),
 *  инкрементирует tick и при превышении лимита обнуляет счётчик и
 *  возвращает 1 (готов к падению). Иначе возвращает 0.
 */
int t_tick_ready(void) {
  int target = tick_limit_for_level(S.level);
  if (S.tick_limit != target) {
    S.tick_limit = target;
  }

  int ready = 0;
  S.tick += 1;
  if (S.tick > S.tick_limit) {
    S.tick = 0;
    ready = 1;
  }
  return ready;
}

/** @brief Сбросить тик‑счётчик (используется в START/FIX). */
void t_tick_reset(void) { S.tick = 0; }

/** @brief Получить текущее состояние FSM. */
TetrisState t_get_state(void) { return S.state; }

/** @brief Установить состояние FSM. */
void t_set_state(TetrisState newState) { S.state = newState; }

/**
 * @brief Очистить матрицу field (буфер для UI).
 * @details Используется при старте партии (STATE_START) и перед перерисовкой
 *          кадра в API, чтобы затем наложить активную фигуру.
 */
void t_clear_field(void) {
  int row = 0;
  while (row < T_ROWS) {
    int col = 0;
    while (col < T_COLS) {
      S.field[row][col] = 0;
      col++;
    }
    row++;
  }
}

/**
 * @brief Скопировать содержимое board → field.
 * @details Поле board хранит зафиксированные блоки; field — буфер для UI,
 *          куда поверх копии накладывается активная фигура.
 */
void t_copy_board_to_field(void) {
  int row = 0;
  while (row < T_ROWS) {
    int col = 0;
    while (col < T_COLS) {
      S.field[row][col] = S.board[row][col];
      col++;
    }
    row++;
  }
}

/**
 * @brief Проверка коллизии ячейки активной фигуры при смещении.
 */
static int blocked_at_cell(int shapeRow, int shapeCol, int dx, int dy) {
  int blocked = 0;
  if (S.act.shape.m[shapeRow][shapeCol] != 0) {
    int nextRow = S.act.y + dy + shapeRow;
    int nextCol = S.act.x + dx + shapeCol;
    if (nextCol < 0 || nextCol >= T_COLS || nextRow >= T_ROWS) {
      blocked = 1;
    } else if (nextRow >= 0 && S.board[nextRow][nextCol] != 0) {
      blocked = 1;
    }
  }
  return blocked;
}

/**
 * @brief Нанести одну ячейку активной фигуры в field (если в границах).
 */
static void blit_cell_to_field(int shapeRow, int shapeCol) {
  if (S.act.shape.m[shapeRow][shapeCol] == 0) {
    return;
  }
  int fieldRow = S.act.y + shapeRow;
  int fieldCol = S.act.x + shapeCol;
  if (fieldRow >= 0 && fieldRow < T_ROWS && fieldCol >= 0 &&
      fieldCol < T_COLS) {
    S.field[fieldRow][fieldCol] = 1;
  }
}

/** @brief Повернуть активную фигуру на 90° против часовой (внутр.). */
static void rotate_active_ccw(void) {
  Shape src = S.act.shape;
  int r = 0;
  while (r < 4) {
    int c = 0;
    while (c < 4) {
      S.act.shape.m[r][c] = src.m[c][3 - r];
      c = c + 1;
    }
    r = r + 1;
  }
}

/** @brief Повернуть активную фигуру на 90° по часовой (внутр.). */
static void rotate_active_cw(void) {
  Shape src = S.act.shape;
  int r = 0;
  while (r < 4) {
    int c = 0;
    while (c < 4) {
      S.act.shape.m[r][c] = src.m[3 - c][r];
      c = c + 1;
    }
    r = r + 1;
  }
}

/**
 * @brief Сформировать field для UI: board + активная фигура.
 */
void t_render_active_to_field(void) {
  t_copy_board_to_field();
  int r = 0;
  while (r < 4) {
    int c = 0;
    while (c < 4) {
      blit_cell_to_field(r, c);
      c = c + 1;
    }
    r = r + 1;
  }
}

/**
 * @brief Проверить, можно ли сдвинуть активную фигуру.
 * @param deltaX Смещение по X (−1/0/+1).
 * @param deltaY Смещение по Y (0/1).
 * @return 1 — можно; 0 — заблокировано.
 */
int t_can_move(int deltaX, int deltaY) {
  int canMove = 1;
  int r = 0;
  while (r < 4) {
    int c = 0;
    while (c < 4) {
      if (blocked_at_cell(r, c, deltaX, deltaY)) {
        canMove = 0;
      }
      c = c + 1;
    }
    r = r + 1;
  }
  return canMove;
}

/**
 * @brief Попробовать сдвинуть активную фигуру.
 * @return 1 — удалось; 0 — нет.
 */
int t_try_move(int deltaX, int deltaY) {
  int moved = 0;
  if (t_can_move(deltaX, deltaY) != 0) {
    S.act.x += deltaX;
    S.act.y += deltaY;
    moved = 1;
  }
  return moved;
}

/**
 * @brief Повернуть активную фигуру по часовой с откатом при коллизии.
 */
void t_rotate_cw(void) {
  rotate_active_cw();
  if (t_can_move(0, 0) == 0) {
    rotate_active_ccw();
  }
}

/**
 * @brief Проверить, можно ли опустить фигуру на 1 клетку вниз.
 */
int t_can_drop(void) {
  int result = t_can_move(0, 1);
  return result;
}

/**
 * @brief Опустить фигуру на 1 клетку (если возможно).
 */
void t_drop_one(void) {
  if (t_can_move(0, 1) != 0) {
    S.act.y += 1;
  }
}

/**
 * @brief Жёсткий дроп: опускает фигуру, пока возможно.
 */
void t_hard_drop(void) {
  int canDropMore = t_can_drop();
  while (canDropMore != 0) {
    S.act.y += 1;
    canDropMore = t_can_drop();
  }
}

/**
 * @brief Приклеить активную фигуру к board и проверить GAMEOVER.
 * @details Переносит занятые ячейки активной фигуры в `board`. Если часть
 *          оказалась выше верхней границы — устанавливает GAMEOVER.
 */
void t_fix_to_board(void) {
  int out_of_top = 0;
  int y = 0;
  while (y < 4) {
    int x = 0;
    while (x < 4) {
      if (S.act.shape.m[y][x] != 0) {
        int ry = S.act.y + y;
        int rx = S.act.x + x;
        if (ry < 0) {
          out_of_top = 1;
        }
        if (ry >= 0 && ry < T_ROWS && rx >= 0 && rx < T_COLS) {
          S.board[ry][rx] = 1;
        }
      }
      x = x + 1;
    }
    y = y + 1;
  }
  if (out_of_top != 0) {
    t_set_state(STATE_GAMEOVER);
  }
}

/** @brief Отметить полностью заполненные строки (внутр.). */
static int mark_full_rows(int fullRow[T_ROWS]) {
  int cleared = 0;
  for (int r = 0; r < T_ROWS; ++r) {
    int full = 1;
    for (int c = 0; c < T_COLS; ++c) {
      if (S.board[r][c] == 0) {
        full = 0;
      }
    }
    fullRow[r] = full;
    if (full) cleared += 1;
  }
  return cleared;
}

/** @brief Сжать поле вниз, пропуская отмеченные строки*/
static int compress_board_down(const int fullRow[T_ROWS]) {
  int writeRow = T_ROWS - 1;
  for (int r = T_ROWS - 1; r >= 0; --r) {
    if (fullRow[r] == 0) {
      for (int c = 0; c < T_COLS; ++c) {
        S.board[writeRow][c] = S.board[r][c];
      }
      writeRow -= 1;
    }
  }
  return writeRow;
}

/** @brief Заполнить верх пустыми строками после сжатия */
static void fill_top_zeros_from(int writeRow) {
  while (writeRow >= 0) {
    for (int c = 0; c < T_COLS; ++c) {
      S.board[writeRow][c] = 0;
    }
    writeRow -= 1;
  }
}

/**
 * @brief Начислить очки, обновить уровень/скорость по числу очищенных линий.
 * @details 1→100, 2→300, 3→700, 4→1500; лимит тиков ускоряется с ростом уровня.
 * @ingroup FSM_FIX
 */
static void apply_scoring_and_level(int cleared) {
  if (cleared > 0) {
    int points = 0;
    if (cleared == 1)
      points = 100;
    else if (cleared == 2)
      points = 300;
    else if (cleared == 3)
      points = 700;
    else
      points = 1500;
    S.score += points;

    int targetLevel = 1 + (S.score / 600);
    if (targetLevel > 10) targetLevel = 10;
    if (targetLevel > S.level) {
      int delta = targetLevel - S.level;
      S.level = targetLevel;
      int newLimit = S.tick_limit - delta;
      if (newLimit < 2) newLimit = 2;
      S.tick_limit = newLimit;
    }

    S.lines_done += cleared;
    if (S.lines_done >= 10) {
      S.level += 1;
      S.lines_done -= 10;
      if (S.tick_limit > 2) S.tick_limit -= 1;
    }
    if (S.score > S.high_score) {
      S.high_score = S.score;
      save_high_score();
    }
  }
}

/**
 * @brief Очистить заполненные линии и обновить скоринг/уровень.
 * @return Количество очищенных линий.
 */
int t_clear_full_lines(void) {
  int fullRow[T_ROWS];
  int cleared = mark_full_rows(fullRow);
  int writeRow = compress_board_down(fullRow);
  fill_top_zeros_from(writeRow);
  apply_scoring_and_level(cleared);
  return cleared;
}

/**
 * @brief Спаун новой активной фигуры из «мешка» 7‑фигур.
 * @return 1 — успешно; 0 — нет места (GAMEOVER).
 * @details Переносит next_id в активную фигуру, берёт новый next_id,
 *  ставит по центру фигуру и ставит y=-1 (над полем). Проверяет
 *          возможность хода вниз/статуса.
 */
int t_spawn_new_piece(void) {
  int ok = 1;

  int current_id = S.next_id;
  if (S.bag_index >= 7) {
    refill_bag();
    S.bag_index = 0;
  }
  S.next_id = S.bag[S.bag_index++];

  Shape nextShape = SHAPES[current_id];

  S.act.shape = nextShape;
  S.act.x = (T_COLS / 2) - 2;
  S.act.y = -1;

  if (t_can_move(0, 1) == 0 && t_can_move(0, 0) == 0) {
    ok = 0;
  }

  return ok;
}

/**
 * @brief Построить 4×4 предпросмотр фигуры `next_id`.
 */
void t_build_next_preview(void) {
  int row = 0;
  while (row < 4) {
    int col = 0;
    while (col < 4) {
      S.next[row][col] = SHAPES[S.next_id].m[row][col];
      col++;
    }
    row++;
  }
}

static const char *HIGHSCORE_FILE = "tetris_highscore.txt";
/** @brief Загрузить рекорд из файла */
static void load_high_score(void) {
  FILE *f = fopen(HIGHSCORE_FILE, "r");
  int value = 0;
  int ok = 0;

  if (f != NULL) {
    if (fscanf(f, "%d", &value) == 1) {
      if (value >= 0) {
        S.high_score = value;
        ok = 1;
      }
    }
    fclose(f);
  }

  if (ok == 0) {
    S.high_score = 0;
  }
}

/** @brief Сохранить рекорд в файл */
static void save_high_score(void) {
  FILE *f = fopen(HIGHSCORE_FILE, "w");
  if (f != NULL) {
    fprintf(f, "%d\n", S.high_score);
    fclose(f);
  }
}

/** @brief Перемешать «мешок» из 7 фигур  */
static void refill_bag(void) {
  for (int i = 0; i < 7; ++i) {
    S.bag[i] = i;
  }
  for (int i = 6; i > 0; --i) {
    int j = rand() % (i + 1);
    int tmp = S.bag[i];
    S.bag[i] = S.bag[j];
    S.bag[j] = tmp;
  }
}
