#include "draw.h"

#include <ncurses.h>
#include <string.h>
#include <time.h>

#include "brick_game_api.h"

/**
 * @brief Точка входа (инициализация ncurses).
 * @details Настраивает режим терминала, запускает главный цикл и корректно
 *          завершает работу ncurses.
 */
void interface_run(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);
  console_loop();
  endwin();
}

/**
 * @brief Главный цикл консольного интерфейса.
 * @details Читает ввод, обрабатывает удержание Action, вызывает шаг отрисовки,
 *          завершает по GAMEOVER/Terminate.
 */
void console_loop(void) {
  int running = 1;
  int quit_overlay = 0;
  int action_down = 0;
  while (running != 0) {
    int last = read_last_keypress();
    process_last_key(last, &action_down, &quit_overlay);
    if (action_down && last != ' ') {
      userInput(Action, 0);
      action_down = 0;
    }
    if (quit_overlay) {
      sleep_ms(250);
      break;
    }
    running = step_and_draw_once();
  }
}

/**
 * @brief Выполнить один шаг обновления и отрисовки.
 * @return 1 — продолжать цикл; 0 — выйти (GAMEOVER/Terminate).
 * @details Получает GameInfo_t, рисует кадр, спит по g.speed (или 50 мс в
 * паузе), освобождает ресурсы через freeGameInfo().
 */
int step_and_draw_once(void) {
  int should_continue = 1;
  GameInfo_t g = updateCurrentState();
  interface_draw(&g);
  if (isGameOver() != 0 || t_take_terminate() != 0) {
    refresh();
    sleep_ms(800);
    should_continue = 0;
  } else {
    int delay = g.speed;
    if (delay < 1) delay = 1;
    if (g.pause == 0)
      sleep_ms(delay);
    else
      sleep_ms(50);
  }
  freeGameInfo(&g);
  return should_continue;
}

/**
 * @brief Полная отрисовка интерфейса кадра (поле, HUD, превью, оверлей).
 * @param g Снимок состояния игры (матрицы/поля/флаги).
 */
void interface_draw(const GameInfo_t *g) {
  int cellw = 2;
  int cellh = 1;
  int top = 1;
  int left = 2;

  clear();

  draw_border_classic(top, left, 20, 10, cellw, cellh);
  draw_matrix_classic(top, left, g ? g->field : 0, 20, 10, cellw, cellh);

  int hud_left = left + 10 * cellw + 4;
  draw_hud_classic(top, hud_left, g);

  if (g && g->next != 0) {
    mvprintw(top + 12, hud_left, "Next:");
    draw_matrix_classic(top + 13, hud_left, g->next, 4, 4, cellw, cellh);
  }

  if (isGameOver() != 0) {
    draw_game_over_banner_over_field(top, left, 20, 10, cellw, cellh);
  }

  refresh();
}

/**
 * @brief Нарисовать прямоугольный «GAME OVER» поверх поля.
 * @param top,left,rows,cols Геометрия поля (в клетках) и позиция.
 * @param cellw,cellh Размер клетки в символах (для расчёта рамки).
 */
void draw_game_over_banner_over_field(int top, int left, int rows, int cols,
                                      int cellw, int cellh) {
  int field_w = cols * cellw + 2;
  int field_h = rows * cellh + 2;
  int fx = left;
  int fy = top;

  const char *title = " GAME OVER ";
  const char *hint = "Press Q to quit";

  int w_title = (int)strlen(title);
  int w_hint = (int)strlen(hint);
  int box_w = w_title > w_hint ? w_title : w_hint;
  box_w += 4;
  if (box_w > field_w - 2) box_w = field_w - 2;

  int box_h = 5;
  if (box_h > field_h - 2) box_h = field_h - 2;

  int x = fx + (field_w - box_w) / 2;
  int y = fy + (field_h - box_h) / 2;

  int scr_rows = 0, scr_cols = 0;
  getmaxyx(stdscr, scr_rows, scr_cols);
  if (x < 1) x = 1;
  if (y < 1) y = 1;
  if (x + box_w >= scr_cols) x = scr_cols - box_w - 1;
  if (y + box_h >= scr_rows) y = scr_rows - box_h - 1;

  attron(A_BOLD);
  mvprintw(y + 0, x + 0, "+");
  for (int i = 0; i < box_w - 2; i++) mvprintw(y + 0, x + 1 + i, "-");
  mvprintw(y + 0, x + box_w - 1, "+");

  mvprintw(y + 1, x + 1, "%-*s", box_w - 2, title);
  mvprintw(y + 2, x + 1, "%-*s", box_w - 2, hint);

  mvprintw(y + 3, x + 0, "+");
  for (int i = 0; i < box_w - 2; i++) mvprintw(y + 3, x + 1 + i, "-");
  mvprintw(y + 3, x + box_w - 1, "+");
  attroff(A_BOLD);
}

/**
 * @brief Нарисовать HUD (Score/Record/Level/Speed и подсказки).
 * @param top,left Позиция HUD в символах.
 * @param g Снимок состояния игры; допускается NULL.
 */
void draw_hud_classic(int top, int left, const GameInfo_t *g) {
  int score = g ? g->score : 0;
  int record = g ? g->high_score : 0;
  int level = g ? g->level : 0;
  int speed_ms = g ? g->speed : 0;
  double factor = (speed_ms > 0) ? (32.0 / (double)speed_ms) : 0.0;

  mvprintw(top + 0, left, "Score : %d", score);
  mvprintw(top + 1, left, "Record: %d", record);
  mvprintw(top + 2, left, "Level : %d", level);
  mvprintw(top + 3, left, "Speed : %d ms (x%.1f)", speed_ms, factor);

  mvprintw(top + 5, left, "Controls:");
  mvprintw(top + 6, left, "  Arrows  - move/rot.");
  mvprintw(top + 7, left, "  Space   - action");
  mvprintw(top + 8, left, "  P       - pause");
  mvprintw(top + 9, left, "  Q/Esc   - quit");
}

extern int isGameOver(void);
extern int t_take_terminate(void);
void freeGameInfo(GameInfo_t *g);

void draw_game_over_banner_over_field(int top, int left, int rows, int cols,
                                      int cellw, int cellh);

void sleep_ms(int ms) {
  if (ms <= 0) return;
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (long)(ms % 1000) * 1000000L;
  nanosleep(&ts, 0);
}

/**
 * @brief Нарисовать прямоугольную рамку вокруг поля.
 * @param top,left Координаты верхнего левого угла рамки в символах.
 * @param rows,cols Размер поля (в клетках).
 * @param cellw,cellh Размер одной клетки в символах.
 * @details Использует символы ncurses (углы/горизонтальные/вертикальные линии).
 */
void draw_border_classic(int top, int left, int rows, int cols, int cellw,
                         int cellh) {
  int width = cols * cellw + 2;
  int height = rows * cellh + 2;

  mvaddch(top, left, ACS_ULCORNER);
  mvaddch(top, left + width - 1, ACS_URCORNER);
  mvaddch(top + height - 1, left, ACS_LLCORNER);
  mvaddch(top + height - 1, left + width - 1, ACS_LRCORNER);

  int x = left + 1;
  while (x < left + width - 1) {
    mvaddch(top, x, ACS_HLINE);
    mvaddch(top + height - 1, x, ACS_HLINE);
    x = x + 1;
  }

  int y = top + 1;
  while (y < top + height - 1) {
    mvaddch(y, left, ACS_VLINE);
    mvaddch(y, left + width - 1, ACS_VLINE);
    y = y + 1;
  }
}

/**
 * @brief Отрисовать матрицу поля/превью в псевдографике.
 * @param top,left Позиция области рисования.
 * @param rows,cols Размер матрицы (клетки).
 * @param cellw,cellh Размер клетки в символах.
 * @details Печатает "[]" для занятых клеток и пробелы — для пустых.
 */
void draw_matrix_classic(int top, int left, int **grid, int rows, int cols,
                         int cellw, int cellh) {
  int y = 0;
  while (y < rows) {
    int x = 0;
    while (x < cols) {
      int val = 0;
      if (grid != 0) {
        val = grid[y][x];
      }
      int px = left + 1 + x * cellw;
      int py = top + 1 + y * cellh;
      if (val != 0) {
        mvprintw(py, px, "[]");
      } else {
        mvprintw(py, px, "  ");
      }
      x = x + 1;
    }
    y = y + 1;
  }
}

/**
 * @brief Сопоставить нажатую клавишу ncurses действию UserAction_t.
 * @param ch Код клавиши из getch().
 * @param[out] act Результирующее действие.
 * @param[out] hold Признак удержания (для Down/Space).
 */
void map_key_to_action(int ch, UserAction_t *act, int *hold) {
  *act = Action;
  *hold = 0;

  if (ch == KEY_LEFT) {
    *act = Left;
  } else if (ch == KEY_RIGHT) {
    *act = Right;
  } else if (ch == KEY_UP) {
    *act = Up;
  } else if (ch == KEY_DOWN) {
    *act = Down;
    *hold = 1;
  } else if (ch == ' ') {
    *act = Action;
    *hold = 1;
  } else if (ch == '\n' || ch == '\r') {
    *act = Start;
  } else if (ch == 'p' || ch == 'P') {
    *act = Pause;
  } else if (ch == 27 || ch == 'q' || ch == 'Q') {
    *act = Terminate;
  }
}

/**
 * @brief Считать последнюю нажатую клавишу, очищая буфер ввода.
 * @return Код последней клавиши или -1, если ввода нет.
 */
int read_last_keypress(void) {
  int ch = getch();
  int last = -1;
  while (ch != ERR) {
    last = ch;
    ch = getch();
  }
  return last;
}

/**
 * @brief Преобразовать последнюю клавишу в действие и отправить в игру.
 * @param last Код клавиши (или -1).
 * @param[in,out] action_down Флаг «Space удерживается» для авто‑повтора.
 * @param[in,out] quit_overlay Флаг запроса выхода (по Terminate).
 * @details Обрабатывает удержание Action (Space), Down (мгновенный сброс hold).
 */
void process_last_key(int last, int *action_down, int *quit_overlay) {
  if (last == -1) return;
  UserAction_t a;
  int hold;
  map_key_to_action(last, &a, &hold);
  if (a == Terminate) {
    *quit_overlay = 1;
  }

  if (a == Action && hold) {
    if (!(*action_down)) {
      userInput(a, hold);
      *action_down = 1;
    }
  } else {
    userInput(a, hold);
    if (a == Down) userInput(a, 0);
  }
}
