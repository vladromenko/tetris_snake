#include "brick_game_api.h"
#include "snake.h"

namespace s21 {

/**
 * @brief Обработчик ввода для змейки.
 * @param action Унифицированное действие пользователя.
 * @param hold   Признак «удержания» действия (для Action — ускорение).
 * @details
 *  Транслирует действия интерфейса в соответствующие вызовы
 *  игрового объекта:
 *   - Left/Right: ставят запрос поворота (применяется один раз в
 * FSM_StepInput()).
 *   - Action: если hold=true — включается удерживаемое ускорение
 *  если hold=false — ускорение снимается и разрешается
 * разовый «рывок» ближайшего шага.
 *   - Pause: переключение паузы.
 *   - Terminate: запрос завершения партии.
 */
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

}  // namespace s21
