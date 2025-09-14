#ifndef VIEW_H
#define VIEW_H

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "brick_game_api.h"


class View : public QWidget {
  Q_OBJECT
 public:
  
  explicit View(QWidget* parent = nullptr);
  
  ~View() override;

 protected:
  
  void paintEvent(QPaintEvent* event) override;
  
  void keyPressEvent(QKeyEvent* event) override;
  
  void keyReleaseEvent(QKeyEvent* event) override;

 private slots:
  
  void onTick();

 private:
  
  void drawBorder(QPainter& p, const QRect& r);
  
  void drawMatrix(QPainter& p, int** grid, int rows, int cols,
                  const QRect& area);
  
  void drawHUD(QPainter& p, const QRect& bounds, const GameInfo_t& g,
               bool game_over);

  
  int rows_ = 20;
  int cols_ = 10;

  
  int margin_out_ = 8;
  int margin_in_ = 6;

  
  bool quit_pending_ = false;

  
  QPointer<QTimer> timer_;
};

#endif  
