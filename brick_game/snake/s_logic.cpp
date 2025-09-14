#include "snake.h"

namespace s21 {

/**
 * @defgroup SnakeFSM Конечный автомат (FSM)
 * @brief Состояния и переходы основной петли игры.
 * @details
 *  Состояния цикла:
 *  - STATE_START: запуск новой партии (сброс флагов, создание тела, размещение
 * еды).
 *  - STATE_INPUT: применение поворота, проверка паузы/завершения.
 *  - STATE_DROP: ожидание лимита тиков до шага; при готовности — переход к
 * FIX.
 *  - STATE_FIX: фиксация шага (движение головы, проверка еды/коллизий),
 * возможен GAMEOVER.
 *  - STATE_PAUSED: пауза; счётчик тиков обнуляется, движение не происходит.
 *  - STATE_GAMEOVER: состояние завершённой партии.
 *
 *  Переходы за 1 вызов Step():
 *  - START → INPUT — сразу после инициализации новой партии.
 *  - INPUT → PAUSED — если включена пауза (`paused_ == true`).
 *  - INPUT → GAMEOVER — если установлен флаг завершения или игра завершена.
 *  - INPUT → DROP — если игра идёт и паузы нет.
 *  - DROP → FIX — как только накоплен требуемый лимит тиков.
 *  - FIX → INPUT — если ход успешно применён (двигались/поели, без коллизии).
 *  - FIX → GAMEOVER — если зафиксирована коллизия со стеной/телом.
 *
 */

/**
 * @brief Внутреннее хранение текущего состояния FSM.
 */
enum SnakeState {
  STATE_START = 0, /**< старт новой партии */
  STATE_INPUT,     /**< обработка входа/поворота */
  STATE_DROP,      /**< ожидание лимита тиков до шага */
  STATE_FIX,       /**< фиксация хода (движение/еда/коллизии) */
  STATE_PAUSED,    /**< пауза */
  STATE_GAMEOVER   /**< игра завершена */
};

/** @brief Возвращает ссылку на статическое состояние FSM.
 */
static SnakeState& SnakeStateRef() {
  static SnakeState state = STATE_START;
  return state;
}

/**
 * @brief Один шаг конечного автомата змейки.
 * @details
 *  1) Сначала учитываем «снятие флага» для долгоживущих состояний:
 *     - если были в GAMEOVER, но game_over_ == false — возвращаемся к START;
 *     - если были в PAUSED, но paused_ == false — возвращаемся к INPUT.
 *  2) Далее выполняется обработчик текущего состояния и выбирается следующее:
 *     - START: FSM_StepStart(); → INPUT.
 *     - INPUT: FSM_StepInput(); далее:
 *         • paused_ → PAUSED;
 *         • game_over_ или TakeTerminateOnce() → GAMEOVER;
 *         • иначе → DROP.
 *     - DROP: если FSM_StepDrop() вернул true (лимит тиков достигнут/рывок) →
 * FIX, иначе остаёмся в DROP (продолжаем копить тики).
 *     - FIX: если FSM_StepFix() вернул true (ход применён без коллизии) →
 * INPUT, иначе → GAMEOVER (коллизия/завершение).
 *     - PAUSED: FSM_StepPaused(); остаёмся в PAUSED до снятия паузы во входе.
 *     - GAMEOVER: FSM_StepGameOver(); остаёмся в GAMEOVER до старта новой
 * партии.
 */
void SnakeGame::Step() {
  SnakeState& st = SnakeStateRef();
  if (st == STATE_GAMEOVER && !game_over_) st = STATE_START;
  if (st == STATE_PAUSED && !paused_) st = STATE_INPUT;

  SnakeState next = st;
  switch (st) {
    case STATE_START:
      FSM_StepStart();
      next = STATE_INPUT;
      break;
    case STATE_INPUT:
      FSM_StepInput();
      if (paused_) {
        next = STATE_PAUSED;
      } else if (game_over_ || TakeTerminateOnce()) {
        next = STATE_GAMEOVER;
      } else {
        next = STATE_DROP;
      }
      break;
    case STATE_DROP:
      if (FSM_StepDrop()) {
        next = STATE_FIX;
      } else {
        next = STATE_DROP;
      }
      break;
    case STATE_FIX:
      if (FSM_StepFix()) {
        next = STATE_INPUT;
      } else {
        next = STATE_GAMEOVER;
      }
      break;
    case STATE_PAUSED:
      FSM_StepPaused();
      next = STATE_PAUSED;
      break;
    case STATE_GAMEOVER:
      FSM_StepGameOver();
      next = STATE_GAMEOVER;
      break;
  }
  st = next;
}

}  // namespace s21
