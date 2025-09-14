#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>

#include "snake.h"

namespace s21 {

/**
 * @brief Инициализировать игру под заданную геометрию поля.
 * @param w ширина поля (клетки)
 * @param h высота поля (клетки)
 * @details
 *  1) InitHighScoreIfNeeded() — загружает рекорд из файла (один раз
 *     за процесс), не понижая значение, если в памяти больше.
 *  2) InitGeometry(w, h) — настраивает размеры поля, сбрасывает уровень,
 *     счёт, базовые тайминги и связанные счётчики.
 *  3) InitRuntimeState() — готовит рантайм новой партии: сбрасывает флаги
 *     (пауза/ускорение/terminate), строит стартовое тело, размещает еду,
 *     обнуляет тик‑счётчик и фиксирует время последнего шага.
 * @return Ничего (void).
 */
void SnakeGame::Init(int w, int h) {
  InitHighScoreIfNeeded();
  InitGeometry(w, h);
  InitRuntimeState();
}

/**
 * @brief Загрузить лучший счёт из файла (один раз за процесс).
 * @details
 *  - Если рекорд уже был загружен ранее, функция
 *    возвращается без дополнительных действий.
 *  - Сохраняет текущее значение рекорда в old_high, затем вызывает
 *    LoadHighScoreFromFile().
 *  - Если считанное из файла значение меньше, чем уже находившееся в памяти,
 *    сохраняет более высокое.
 *  - Устанавливает high_loaded_ = true, чтобы больше не перечитывать файл.
 */
void SnakeGame::InitHighScoreIfNeeded() {
  if (high_loaded_) return;
  int old_high = high_score_;
  LoadHighScoreFromFile();
  if (high_score_ < old_high) high_score_ = old_high;
  high_loaded_ = true;
}

/**
 * @brief Настроить геометрию поля и сбросить базовые счётчики.
 * @param w ширина поля (клетки)
 * @param h высота поля (клетки)
 * @details
 *  - Устанавливает `width_ = w`, `height_ = h`.
 *  - Сбрасывает уровень и счёт: `level_ = 1`, `score_ = 0`.
 *  - Инициализирует лимиты тиков: `tick_limit_base_ = kDefaultTickBase`
 *    (с ограничением не ниже 1) и `tick_limit_fast_ = kDefaultTickFast`
 *    (также не ниже 1).
 *  - Обнуляет счётчик тиков (`tick_counter_ = 0`).
 *  - Инициализирует счётчики/лимиты тиков (базовый/быстрый) для шага змейки.
 *
 * @return Ничего (void).
 */
void SnakeGame::InitGeometry(int w, int h) {
  width_ = w;
  height_ = h;
  level_ = 1;
  score_ = 0;
  tick_limit_base_ = kDefaultTickBase;
  if (tick_limit_base_ < 1) tick_limit_base_ = 1;
  tick_limit_fast_ = kDefaultTickFast;
  if (tick_limit_fast_ < 1) tick_limit_fast_ = 1;
  tick_counter_ = 0;
}

/**
 * @brief Рекомендуемая задержка кадра UI в миллисекундах.
 * @details
 *  - Вычисляется из текущего уровня: базовое значение 32.0 мс.
 *  - Эта величина предназначена для интерфейса: C‑API кладёт её в
 *    GameInfo_t.speed,
 *      • в консольном UI (draw.c) она используется как пауза между кадрами
 *        и также отображается в HUD,
 *      • в десктопном UI (view.cpp) отображается в HUD (таймер отрисовки
 * фиксированный).
 */
int SnakeGame::SpeedMs() const {
  int lvl = level_ < 1 ? 1 : level_;
  double ms = 32.0 / std::pow(1.5, static_cast<double>(lvl - 1));
  if (is_accelerating_) ms /= 1.5;
  int out = static_cast<int>(std::lround(ms));
  if (out < 8) out = 8;
  return out;
}

/**
 * @brief Подготовить игру новой партии на уже заданной геометрии.
 * @details
 *  1) Сбрасывает флаги (паузу, ускорение, terminate, game over).
 *  2) Формирует стартовое тело змейки в центре поля.
 *  3) Выставляет еду в свободную клетку (детерминированно от текущей головы и
 * счёта). 4) Обнуляет счётчик тиков и фиксирует момент последнего шага.
 */
void SnakeGame::InitRuntimeState() {
  ResetRuntimeFlags();
  InitBodyStart();
  InitFoodFromSeed(body_.front());
  tick_counter_ = 0;
  last_move_tp_ = std::chrono::steady_clock::now();
}

/**
 * @brief Сбросить «быстрые» флаги управления и состояния.
 * @details Устанавливает начальное направление, снимает запрос поворота,
 *          выключает ускорение и «рывок», снимает паузу, очищает признаки
 *          завершения/terminate.
 */
void SnakeGame::ResetRuntimeFlags() {
  current_direction_ = Direction::kRight;
  pending_turn_ = TurnRequest::kNone;
  is_accelerating_ = false;
  accelerate_step_ = false;
  paused_ = false;
  game_over_ = false;
  terminate_requested_ = false;
}

/**
 * @brief Сформировать стартовое тело змейки по центру поля.
 * @details Очищает текущее тело и добавляет четыре сегмента: голову в центре
 *          и три сегмента влево от неё (змейка направлена вправо).
 */
void SnakeGame::InitBodyStart() {
  body_.clear();
  Point head{width_ / 2, height_ / 2};
  body_.push_front(head);
  body_.push_back(Point{head.x - 1, head.y});
  body_.push_back(Point{head.x - 2, head.y});
  body_.push_back(Point{head.x - 3, head.y});
}

/**
 * @brief Разместить еду в свободной клетке, вычислив позицию от seed.
 * @param seed опорная точка (текущая голова)
 * @details Вычисляет начальную позицию (nx, ny) как функции от seed и счёта,
 *          далее линейным проходом ищет первую свободную клетку (не занято
 *          телом), учитывая размеры поля. Хранит найденную позицию в `food_`.
 */
void SnakeGame::InitFoodFromSeed(const Point& seed) {
  int nx = (std::abs(seed.x * 31 + seed.y * 17 + score_ * 13)) % width_;
  int ny = (std::abs(seed.x * 7 + seed.y * 11 + score_ * 5)) % height_;
  bool found = false;
  int total = width_ * height_;
  for (int k = 0; k < total; ++k) {
    int x = (nx + k) % width_;
    int y = (ny + k / width_) % height_;
    bool occ = false;
    for (const auto& b : body_) {
      if (b.x == x && b.y == y) {
        occ = true;
      }
    }
    if (!occ && !found) {
      food_.x = x;
      food_.y = y;
      found = true;
    }
  }
}

/**
 * @brief Рассчитать позицию головы после следующего шага.
 * @details На основе текущего направления устанавливает смещение (dx, dy)
 *          и прибавляет его к текущим координатам головы.
 * @return Новая точка головы (ещё без проверки коллизий/границ).
 */
Point SnakeGame::NextHeadPoint() const {
  Point h = body_.front();
  int dx = 0, dy = 0;
  if (current_direction_ == Direction::kLeft)
    dx = -1;
  else if (current_direction_ == Direction::kRight)
    dx = 1;
  else if (current_direction_ == Direction::kUp)
    dy = -1;
  else
    dy = 1;
  return Point{h.x + dx, h.y + dy};
}

/**
 * @brief Повернуть направление на 90° влево.
 * @param d Текущее направление движения.
 * @return Новое направление после поворота влево.
 * @details Простое сопоставление значений enum: Up→Left→Down→Right→Up.
 */

static Direction RotateLeft(Direction d) {
  Direction out = Direction::kUp;
  if (d == Direction::kUp)
    out = Direction::kLeft;
  else if (d == Direction::kLeft)
    out = Direction::kDown;
  else if (d == Direction::kDown)
    out = Direction::kRight;
  else
    out = Direction::kUp;
  return out;
}

/**
 * @brief Повернуть направление на 90° вправо.
 * @param d Текущее направление движения.
 * @return Новое направление после поворота вправо.
 */
static Direction RotateRight(Direction d) {
  Direction out = Direction::kUp;
  if (d == Direction::kUp)
    out = Direction::kRight;
  else if (d == Direction::kRight)
    out = Direction::kDown;
  else if (d == Direction::kDown)
    out = Direction::kLeft;
  else
    out = Direction::kUp;
  return out;
}

void SnakeGame::RequestTurnLeft() {
  if (pending_turn_ == TurnRequest::kNone) pending_turn_ = TurnRequest::kLeft;
}

void SnakeGame::RequestTurnRight() {
  if (pending_turn_ == TurnRequest::kNone) pending_turn_ = TurnRequest::kRight;
}

/**
 * @brief Проверить, окажется ли голова на еде в точке p.
 * @param p Клетка, куда предполагается шаг головы.
 * @return true, если p совпадает с текущей позицией еды; иначе false.
 */
bool SnakeGame::WillEatAt(const Point& p) const {
  return p.x == food_.x && p.y == food_.y;
}

/**
 * @brief Определить, будет ли столкновение при шаге в точку p.
 * @param p Целевая клетка для головы.
 * @param will_eat Признак, съедается ли еда этим шагом.
 * @return true — столкновение (стена или тело), false — ход безопасен.
 */
bool SnakeGame::DetectCollisionAt(const Point& p, bool will_eat) const {
  bool collides = false;
  if (p.x < 0 || p.x >= width_ || p.y < 0 || p.y >= height_) {
    collides = true;
  } else {
    size_t limit = body_.size();
    if (!will_eat && limit > 0) limit -= 1;
    bool hit_body = false;
    for (size_t i = 0; i < limit; ++i) {
      if (body_[i].x == p.x && body_[i].y == p.y) {
        hit_body = 1;
      }
    }
    collides = hit_body;
  }
  return collides;
}

/**
 * @brief Применить отложенный поворот один раз за тик.
 * @details
 *  - Если pending_turn_ установлен (kLeft/kRight) — вычисляем
 *    через RotateLeft/RotateRight относительно текущего направления.
 */
void SnakeGame::ApplyPendingTurnOnce() {
  if (pending_turn_ == TurnRequest::kNone) return;
  Direction cand = current_direction_;
  if (pending_turn_ == TurnRequest::kLeft)
    cand = RotateLeft(current_direction_);
  else if (pending_turn_ == TurnRequest::kRight)
    cand = RotateRight(current_direction_);
  if (!IsOpposite(cand, current_direction_)) current_direction_ = cand;
  pending_turn_ = TurnRequest::kNone;
}

void SnakeGame::MaybeLevelUp() {
  int new_level = 1 + score_ / 5;
  if (new_level < 1) new_level = 1;
  if (new_level > 10) new_level = 10;
  if (new_level != level_) {
    level_ = new_level;
    double factor = std::pow(1.5, static_cast<double>(level_ - 1));
    int tl = static_cast<int>(
        std::lround(static_cast<double>(kDefaultTickBase) / factor));
    if (tl < 2) tl = 2;
    tick_limit_base_ = tl;
    int fl = static_cast<int>(std::lround(tick_limit_base_ / 2.0));
    if (fl < 1) fl = 1;
    tick_limit_fast_ = fl;
  }
}

/**
 * @brief Размещает новую еду на свободной клетке поля.
 *
 * @details
 *  Алгоритм вычисляет стартовую точку сканирования поля
 *  из текущей позиции еды и счёта (через две еды s1 и s2), после чего
 *  выполняет обход по всем клеткам. Первая встретившаяся свободная
 *  клетка выбирается под еду.
 */
void SnakeGame::SpawnFoodNext() {
  int s1 = food_.x + food_.y + score_;
  int s2 = food_.x * 31 + food_.y * 17 + score_ * 13;

  int nx = (std::abs(s1) + std::abs(s2)) % width_;
  int ny = (std::abs(s1 * 7) + std::abs(s2 * 11)) % height_;

  bool found = false;
  int total = width_ * height_;

  for (int k = 0; k < total; ++k) {
    int x = (nx + k) % width_;

    int y = (ny + k / width_) % height_;

    bool occ = false;
    for (const auto& b : body_) {
      if (b.x == x && b.y == y) {
        occ = true;
      }
    }

    if (!occ && !found) {
      food_.x = x;
      food_.y = y;
      found = true;
    }
  }
}

void SnakeGame::ApplyMoveOrEat(const Point& p, bool will_eat) {
  body_.push_front(p);
  if (will_eat) {
    score_ += 1;
    if (high_score_ < score_) {
      high_score_ = score_;
      SaveHighScoreToFile();
    }
    MaybeLevelUp();
    SpawnFoodNext();
  } else {
    body_.pop_back();
  }
}

bool SnakeGame::TakeTerminateOnce() {
  bool result = false;
  if (terminate_requested_) {
    result = true;
    terminate_requested_ = false;
  }
  return result;
}

void SnakeGame::LoadHighScoreFromFile() {
  int loaded = 0;
  std::ifstream fin(highscore_path_);
  if (fin.good()) {
    long long tmp = 0;
    fin >> tmp;
    if (!fin.fail()) {
      if (tmp < 0) tmp = 0;
      if (tmp > std::numeric_limits<int>::max())
        tmp = std::numeric_limits<int>::max();
      high_score_ = static_cast<int>(tmp);
      loaded = 1;
    }
  }
  if (!loaded) {
    high_score_ = 0;
  }
}

void SnakeGame::SaveHighScoreToFile() const {
  std::ofstream fout(highscore_path_, std::ios::trunc);
  if (fout.good()) fout << high_score_ << "\n";
}

/**
 * @brief Обработчик состояния START: запустить новую партию.
 * @details Вызывает InitRuntimeState() — сбрасывает флаги, строит тело,
 *          размещает еду, обнуляет тик‑счётчик и фиксирует время.
 */
void SnakeGame::FSM_StepStart() { InitRuntimeState(); }

/**
 * @brief Обработчик состояния INPUT: применить поворот.
 * @details Если в предыдущих шагах был запрошен поворот (Left/Right),
 *          он применяется здесь (с проверкой на противоположное направление).
 */
void SnakeGame::FSM_StepInput() { ApplyPendingTurnOnce(); }

/**
 * @brief Шаг «ожидания тика» перед движением змейки.
 * @details Идея: змейка двигается не на каждом кадре, а
 *          через фиксированное количество «тиков». На каждом вызове:
 *  - Если ранее был задан «рывок» (ClickAccelerate), двигаемся сразу,
 *    обнуляя счётчик тиков.
 *  - Иначе берём лимит тиков (EffectiveTickLimit):
 *      • без ускорения — kDefaultTickBase (=5),
 *      • с удержанием ускорения — kDefaultTickFast (=3).
 *
 * @return true — пора двигаться (переход к FIX), false — ждём ещё (остаёмся в
 * DROP).
 */
bool SnakeGame::FSM_StepDrop() {
  bool ready = false;
  if (accelerate_step_) {
    accelerate_step_ = false;
    tick_counter_ = 0;
    last_move_tp_ = std::chrono::steady_clock::now();
    ready = true;
  } else {
    int limit = EffectiveTickLimit();
    if (tick_counter_ + 1 >= limit) {
      tick_counter_ = 0;
      last_move_tp_ = std::chrono::steady_clock::now();
      ready = true;
    } else {
      tick_counter_ += 1;
      ready = false;
    }
  }
  return ready;
}

bool SnakeGame::FSM_StepFix() {
  Point next = NextHeadPoint();
  bool eat = WillEatAt(next);
  bool ok = true;
  if (DetectCollisionAt(next, eat)) {
    game_over_ = true;
    terminate_requested_ = true;
    is_accelerating_ = false;
    accelerate_step_ = false;
    ok = false;
  } else {
    ApplyMoveOrEat(next, eat);
    ok = true;
  }
  return ok;
}

void SnakeGame::FSM_StepPaused() { tick_counter_ = 0; }

void SnakeGame::FSM_StepGameOver() {}

bool SnakeGame::IsOpposite(Direction a, Direction b) const {
  bool opp = false;
  if ((a == Direction::kUp && b == Direction::kDown) ||
      (a == Direction::kDown && b == Direction::kUp) ||
      (a == Direction::kLeft && b == Direction::kRight) ||
      (a == Direction::kRight && b == Direction::kLeft)) {
    opp = true;
  }
  return opp;
}

/**
 * @brief Конструктор класса SnakeGame.
 * @details
 *  1) Что это: начальная инициализация полей объекта до вызова Init().
 *  2) Зачем нужно: чтобы объект имел корректные стартовые значения
 *     (Размеры по умолчанию 10×20, направление вправо, флаги/счётчики в нуле,
 *     базовые лимиты тиков, пути к файлу рекорда и т.п.).
 *  3) Внутри: инициализирует все поля класса значениями по умолчанию.
 *  4) Если убрать: поля останутся неинициализированными — последующая логика
 *     Init()/Step() может работать некорректно или с неопределённым поведением.
 */
SnakeGame::SnakeGame()
    : width_(10),
      height_(20),
      current_direction_(Direction::kRight),
      pending_turn_(TurnRequest::kNone),
      is_accelerating_(false),
      accelerate_step_(false),
      paused_(false),
      game_over_(false),
      terminate_requested_(false),
      tick_limit_base_(kDefaultTickBase),
      tick_limit_fast_(kDefaultTickFast),
      tick_counter_(0),
      level_(1),
      score_(0),
      high_score_(0),
      food_{0, 0},
      highscore_path_("snake_highscore.txt"),
      high_loaded_(false),
      last_move_tp_(std::chrono::steady_clock::now()) {
  std::srand(static_cast<unsigned>(std::time(nullptr)));
}

/**
 * @brief Включить/выключить удерживаемое ускорение.
 * @param on 1 — включить ускорение, 0 — выключить
 * @details Влияет на лимит тиков до шага (EffectiveTickLimit): при on=true
 *          используется быстрый лимит.
 */
void SnakeGame::SetAcceleration(bool on) { is_accelerating_ = on; }
/**
 * @brief Однократный «рывок» — разрешить ближайший шаг немедленно.
 * @details Ставит флаг, который считывается в FSM_StepDrop(): при наличии
 *         рывка шаг разрешается независимо от текущего tick_counter_.
 */
void SnakeGame::ClickAccelerate() { accelerate_step_ = true; }
/**
 * @brief Переключить паузу (снять/установить).
 * @details Инвертирует флаг paused_. В Step() это отражается переходом
 *          между INPUT и PAUSED.
 */
void SnakeGame::TogglePause() { paused_ = !paused_; }
/**
 * @brief Запросить завершение партии.
 * @details Ставит флаг, который будет считан однократно через
 *          TakeTerminateOnce() и переведёт игру в GAMEOVER.
 */
void SnakeGame::RequestTerminate() { terminate_requested_ = true; }

/** @brief Ширина поля (клетки). */
int SnakeGame::Width() const { return width_; }
/** @brief Высота поля (клетки). */
int SnakeGame::Height() const { return height_; }
/** @brief Текущее тело змейки (deque от головы к хвосту). */
const std::deque<Point>& SnakeGame::Body() const { return body_; }
/** @brief Позиция еды. */
Point SnakeGame::Food() const { return food_; }
/** @brief Текущий счёт. */
int SnakeGame::Score() const { return score_; }
/** @brief Текущий рекорд. */
int SnakeGame::HighScore() const { return high_score_; }
/** @brief Текущий уровень. */
int SnakeGame::Level() const { return level_; }

/**
 * @brief Вернуть лимит тиков до одного шага змейки.
 * @details Если удерживается ускорение — возвращает быстрый лимит,
 *          иначе базовый.
 */
int SnakeGame::EffectiveTickLimit() const {
  int limit = tick_limit_base_;
  if (is_accelerating_) limit = tick_limit_fast_;
  if (limit < 1) limit = 1;
  return limit;
}

/** @brief Признак паузы. */
bool SnakeGame::Paused() const { return paused_; }
/** @brief Признак завершения партии. */
bool SnakeGame::GameOver() const { return game_over_; }

}  // namespace s21

namespace s21 {
SnakeGame& GlobalSnake() {
  static SnakeGame instance;
  return instance;
}
}  // namespace s21
