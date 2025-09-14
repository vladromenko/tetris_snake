#include "brick_game_api.h"
#include "snake.h"

namespace snake {

void SnakeHandleInput(UserAction_t action, bool hold) {
  if (action == Left) {
    GlobalSnake().RequestTurnLeft();
  } else if (action == Right) {
    GlobalSnake().RequestTurnRight();
  }

  if (action == Action) {
    if (hold) {
      GlobalSnake().SetAcceleration(true);
    } else {
      GlobalSnake().SetAcceleration(false);
      GlobalSnake().ClickAccelerate();
    }
  }

  if (action == Pause) {
    GlobalSnake().TogglePause();
  }

  if (action == Terminate) {
    GlobalSnake().RequestTerminate();
  }
}

}  // namespace snake
