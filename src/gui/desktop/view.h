#ifndef VIEW_H
#define VIEW_H

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "brick_game_api.h"

/**
 * @brief Простейшее Qt‑окно для отрисовки игр (CLI API → Desktop GUI).
 * @details
 *  Поллинг состояния выполняется QTimer'ом через C‑API (`updateCurrentState`).
 *  Рисует матрицу поля, HUD и служебные метки. Клавиши переводятся в
 *  `UserAction_t` и передаются в `userInput`.
 */
class View : public QWidget {
  Q_OBJECT
 public:
  /** @brief Создать виджет представления. */
  explicit View(QWidget* parent = nullptr);
  /** @brief Остановить таймер и разрушить виджет. */
  ~View() override;

 protected:
  /** @brief Отрисовка кадра. */
  void paintEvent(QPaintEvent* event) override;
  /** @brief Обработка нажатия клавиш. */
  void keyPressEvent(QKeyEvent* event) override;
  /** @brief Обработка отпускания клавиш. */
  void keyReleaseEvent(QKeyEvent* event) override;

 private slots:
  /** @brief Таймерный тик — запрос перерисовки, проверка GAMEOVER. */
  void onTick();

 private:
  /** @brief Нарисовать внешнюю рамку. */
  void drawBorder(QPainter& p, const QRect& r);
  /** @brief Нарисовать матрицу клеток внутри заданной области. */
  void drawMatrix(QPainter& p, int** grid, int rows, int cols,
                  const QRect& area);
  /** @brief Нарисовать панель HUD со сводной информацией. */
  void drawHUD(QPainter& p, const QRect& bounds, const GameInfo_t& g,
               bool game_over);

  /** @brief Размеры поля по умолчанию (строки/столбцы). */
  int rows_ = 20;
  int cols_ = 10;

  /** @brief Внешний и внутренний отступы рамки. */
  int margin_out_ = 8;
  int margin_in_ = 6;

  /** @brief Флаг отложенного выхода после GAMEOVER. */
  bool quit_pending_ = false;

  /** @brief Таймер опроса состояния/перерисовки. */
  QPointer<QTimer> timer_;
};

#endif  // VIEW_H
