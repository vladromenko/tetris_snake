#include "tetris.h"

/**
 * @brief Внутреннее состояние ввода тетриса.
 * @details
 *  - hasAction: есть ли отложенное действие для чтения.
 *  - currentAction: сохранённое действие.
 *  - isHoldDown: удерживается ли «вниз» (ускоренное падение).
 *  - terminateRequested: был ли запрошен Terminate.
 */
typedef struct {
  int hasAction;
  UserAction_t currentAction;
  int isHoldDown;
  int terminateRequested;
} TInputState;

/** @brief Доступ к единственному экземпляру состояния ввода. */
static TInputState *t_input_state(void) {
  static TInputState st = {0};
  return &st;
}
#define IS (*t_input_state())

/**
 * @brief Принять действие от UI и обновить состояние ввода.
 * @param action Действие пользователя (см. UserAction_t).
 * @param hold   Признак удержания: 1 — удерживается, 0 — «клик/отпускание».
 * @details
 *  - Для Down обновляет флаг быстрого падения.
 *  - Для Terminate помечает однократный запрос завершения
 *  - Сохраняет последнее действие и выставляет признак hasAction.
 */
void handle_input(UserAction_t action, int hold) {
  if (action == Down) {
    IS.isHoldDown = (hold != 0) ? 1 : 0;
  }

  if (action == Terminate) {
    IS.terminateRequested = 1;
  }

  IS.currentAction = action;
  IS.hasAction = 1;
}

/**
 * @brief Считать одно отложенное действие (если есть).
 * @param[out] outAction Куда записать считанное действие.
 * @return 1 — действие считано и сброшено, 0 — действий нет.
 * @details Если `hasAction` установлен, запись `currentAction` в `*outAction`,
 *          затем `hasAction` сбрасывается. Если действий нет — возвращает 0.
 */
int t_take(UserAction_t *outAction) {
  int result = 0;

  if (outAction != 0) {
    if (IS.hasAction != 0) {
      *outAction = IS.currentAction;
      IS.hasAction = 0;
      result = 1;
    }
  }

  return result;
}

/**
 * @brief обёртка для тестов.
 */
int t_take_test(UserAction_t *out) { return t_take(out); }

/**
 * @brief Признак удерживаемого быстрого падения.
 * @return 1 — Down удерживается; 0 — нет.
 */
int t_is_fast_drop(void) {
  int result = 0;
  if (IS.isHoldDown != 0) {
    result = 1;
  }
  return result;
}

/**
 * @brief Однократно считать запрос Terminate.
 * @return 1 — был запрос (и он сброшен); 0 — не было.
 * @details Если terminateRequested установлен, сбрасывает его и, при
 *          необходимости, отменяет отложенное действие Terminate в буфере,
 *          чтобы не продублировать обработку.
 */
int t_take_terminate(void) {
  int result = 0;
  if (IS.terminateRequested != 0) {
    IS.terminateRequested = 0;
    if (IS.hasAction != 0 && IS.currentAction == Terminate) {
      IS.hasAction = 0;
    }
    result = 1;
  }
  return result;
}

/**
 * @brief Сбросить состояние ввода (при старте новой партии).
 */
void t_input_reset(void) {
  IS.hasAction = 0;
  IS.currentAction = Start;
  IS.isHoldDown = 0;
  IS.terminateRequested = 0;
}
