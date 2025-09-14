#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>

#include "snake.h"

namespace snake {

void SnakeGame::Init(int w, int h) {
  InitHighScoreIfNeeded();
  InitGeometry(w, h);
  InitRuntimeState();
}


void SnakeGame::InitHighScoreIfNeeded() {
  if (high_loaded_) return;
  int old_high = high_score_;
  LoadHighScoreFromFile();
  if (high_score_ < old_high) high_score_ = old_high;
  high_loaded_ = true;
}


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


int SnakeGame::SpeedMs() const {
  int lvl = level_ < 1 ? 1 : level_;
  double ms = 32.0 / std::pow(1.5, static_cast<double>(lvl - 1));
  if (is_accelerating_) ms /= 1.5;
  int out = static_cast<int>(std::lround(ms));
  if (out < 8) out = 8;
  return out;
}


void SnakeGame::InitRuntimeState() {
  ResetRuntimeFlags();
  InitBodyStart();
  InitFoodFromSeed(body_.front());
  tick_counter_ = 0;
  last_move_tp_ = std::chrono::steady_clock::now();
}


void SnakeGame::ResetRuntimeFlags() {
  current_direction_ = Direction::kRight;
  pending_turn_ = TurnRequest::kNone;
  is_accelerating_ = false;
  accelerate_step_ = false;
  paused_ = false;
  game_over_ = false;
  terminate_requested_ = false;
}


void SnakeGame::InitBodyStart() {
  body_.clear();
  Point head{width_ / 2, height_ / 2};
  body_.push_front(head);
  body_.push_back(Point{head.x - 1, head.y});
  body_.push_back(Point{head.x - 2, head.y});
  body_.push_back(Point{head.x - 3, head.y});
}


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


bool SnakeGame::WillEatAt(const Point& p) const {
  return p.x == food_.x && p.y == food_.y;
}


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


void SnakeGame::FSM_StepStart() { InitRuntimeState(); }


void SnakeGame::FSM_StepInput() { ApplyPendingTurnOnce(); }


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


void SnakeGame::SetAcceleration(bool on) { is_accelerating_ = on; }

void SnakeGame::ClickAccelerate() { accelerate_step_ = true; }

void SnakeGame::TogglePause() { paused_ = !paused_; }

void SnakeGame::RequestTerminate() { terminate_requested_ = true; }


int SnakeGame::Width() const { return width_; }

int SnakeGame::Height() const { return height_; }

const std::deque<Point>& SnakeGame::Body() const { return body_; }

Point SnakeGame::Food() const { return food_; }

int SnakeGame::Score() const { return score_; }

int SnakeGame::HighScore() const { return high_score_; }

int SnakeGame::Level() const { return level_; }


int SnakeGame::EffectiveTickLimit() const {
  int limit = tick_limit_base_;
  if (is_accelerating_) limit = tick_limit_fast_;
  if (limit < 1) limit = 1;
  return limit;
}


bool SnakeGame::Paused() const { return paused_; }

bool SnakeGame::GameOver() const { return game_over_; }

SnakeGame& GlobalSnake() {
  static SnakeGame instance;
  return instance;
}

}  // namespace snake
