#ifndef SNAKE_H_
#define SNAKE_H_

#include <chrono>
#include <deque>
#include <string>

namespace snake {

struct Point {
  int x;
  int y;
};

enum class Direction { kUp, kDown, kLeft, kRight };
enum class TurnRequest { kNone, kLeft, kRight };

class SnakeGame {
 public:
  SnakeGame();

  void Init(int w, int h);
  void Step();

  void RequestTurnLeft();
  void RequestTurnRight();
  void SetAcceleration(bool on);
  void ClickAccelerate();
  void TogglePause();
  void RequestTerminate();
  bool TakeTerminateOnce();

  int Width() const;
  int Height() const;
  const std::deque<Point>& Body() const;
  Point Food() const;
  int Score() const;
  int HighScore() const;
  int Level() const;

  int EffectiveTickLimit() const;
  bool Paused() const;
  bool GameOver() const;

  int SpeedMs() const;

  void FSM_StepStart();
  void FSM_StepInput();

  bool FSM_StepDrop();
  bool FSM_StepFix();
  void FSM_StepPaused();
  void FSM_StepGameOver();

 private:
  void ApplyPendingTurnOnce();
  Point NextHeadPoint() const;
  bool WillEatAt(const Point& p) const;
  bool DetectCollisionAt(const Point& p, bool will_eat) const;
  void ApplyMoveOrEat(const Point& p, bool will_eat);
  void MaybeLevelUp();
  void LoadHighScoreFromFile();
  void SaveHighScoreToFile() const;

  void InitHighScoreIfNeeded();
  void InitGeometry(int w, int h);
  void ResetRuntimeFlags();
  void InitBodyStart();
  void InitFoodFromSeed(const Point& seed);
  void SpawnFoodNext();
  void InitRuntimeState();

  bool IsOpposite(Direction a, Direction b) const;

 private:
  int width_;
  int height_;
  std::deque<Point> body_;
  Direction current_direction_;
  TurnRequest pending_turn_;
  bool is_accelerating_;
  bool accelerate_step_;
  bool paused_;
  bool game_over_;
  bool terminate_requested_;
  int tick_limit_base_;
  int tick_limit_fast_;
  int tick_counter_;
  int level_;
  int score_;
  int high_score_;
  Point food_;
  std::string highscore_path_;
  bool high_loaded_;

  std::chrono::steady_clock::time_point last_move_tp_;

  static constexpr int kDefaultTickBase = 5;
  static constexpr int kDefaultTickFast = 3;
};

SnakeGame& GlobalSnake();
}  // namespace snake

#define g_snake snake::GlobalSnake()

#endif
