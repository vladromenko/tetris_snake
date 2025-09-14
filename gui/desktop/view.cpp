#include "view.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QTimer>


void View::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, false);

  const QRect bounds =
      rect().adjusted(margin_out_, margin_out_, -margin_out_, -margin_out_);
  drawBorder(p, bounds);

  GameInfo_t g = updateCurrentState();

  const int hudWidth = qMax(120, bounds.width() / 3);
  const QRect boardArea =
      QRect(bounds.left() + margin_in_, bounds.top() + margin_in_,
            bounds.width() - hudWidth - 2 * margin_in_,
            bounds.height() - 2 * margin_in_);
  const QRect hudArea =
      QRect(boardArea.right() + margin_in_, bounds.top() + margin_in_,
            hudWidth - 2 * margin_in_, bounds.height() - 2 * margin_in_);

  int rows = rows_, cols = cols_;

  if (g.field) {
    drawMatrix(p, g.field, rows, cols, boardArea);
  } else {
    p.fillRect(boardArea, QColor(18, 18, 18));
  }

  const bool game_over = isGameOver();
  drawHUD(p, hudArea, g, game_over);

  if (g.next) {
    p.save();
    p.setPen(QColor(200, 200, 200));
    QFont f = p.font();
    f.setBold(true);
    p.setFont(f);
    const int pad = 8;
    const int availW = hudArea.width() - 2 * pad;
    const int availH = hudArea.height() - 2 * pad;

    const int boardCellW = qMax(1, boardArea.width() / qMax(1, cols));
    const int boardCellH = qMax(1, boardArea.height() / qMax(1, rows));
    const int boardCell = qMin(boardCellW, boardCellH);

    int pvCell = boardCell;
    pvCell = qMin(pvCell, availW / 4);
    pvCell = qMin(pvCell, availH / 4);
    if (pvCell < 1) pvCell = 1;

    const int size = pvCell * 4;
    const int nx = hudArea.left() + (hudArea.width() - size) / 2;
    const int ny = hudArea.top() + (hudArea.height() - size) / 2;
    const QRect nextArea(nx, ny, size, size);
    p.drawText(QRect(hudArea.left(), ny - 18, hudArea.width(), 16),
               Qt::AlignHCenter, "NEXT");
    drawMatrix(p, g.next, 4, 4, nextArea);
    p.restore();
  }

  freeGameInfo(&g);

  if (quit_pending_) {
  }
}


void View::drawBorder(QPainter& p, const QRect& r) {
  p.save();
  p.setPen(QPen(QColor(90, 90, 90), 2));
  p.drawRect(r.adjusted(0, 0, -1, -1));
  p.restore();
}


void View::drawMatrix(QPainter& p, int** grid, int rows, int cols,
                      const QRect& area) {
  p.save();
  p.fillRect(area, QColor(18, 18, 18));

  if (rows <= 0 || cols <= 0) {
    p.restore();
    return;
  }

  const int cellW = qMax(1, area.width() / cols);
  const int cellH = qMax(1, area.height() / rows);
  const int cell = qMin(cellW, cellH);

  const int offsetX = area.left() + (area.width() - cell * cols) / 2;
  const int offsetY = area.top() + (area.height() - cell * rows) / 2;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const int v = grid[r][c];
      QRect rc(offsetX + c * cell, offsetY + r * cell, cell, cell);
      if (v) {
        p.fillRect(rc.adjusted(1, 1, -1, -1), QColor(0x4C, 0xAF, 0x50));
      } else {
        p.fillRect(rc.adjusted(1, 1, -1, -1), QColor(30, 30, 30));
        p.setPen(QColor(45, 45, 45));
        p.drawRect(rc.adjusted(1, 1, -2, -2));
      }
    }
  }

  p.restore();
}


void View::drawHUD(QPainter& p, const QRect& bounds, const GameInfo_t& g,
                   bool game_over) {
  p.save();
  p.fillRect(bounds, QColor(10, 10, 10));

  p.setPen(QColor(200, 200, 200));
  QFont f = p.font();
  f.setBold(true);
  p.setFont(f);

  int y = bounds.top() + 8;
  const int dy = 22;

  auto line = [&](const QString& k, long long v) {
    p.drawText(bounds.left() + 8, y, QString("%1: %2").arg(k).arg(v));
    y += dy;
  };

  line("SCORE", g.score);
  line("HIGH", g.high_score);
  line("LEVEL", g.level);
  line("SPEED", g.speed);

  if (g.pause) {
    p.setPen(QColor(255, 215, 0));
    p.drawText(bounds.adjusted(0, 80, 0, 0), Qt::AlignHCenter, "PAUSED");
  }
  if (game_over) {
    p.setPen(QColor(255, 80, 80));
    p.drawText(bounds.adjusted(0, 110, 0, 0), Qt::AlignHCenter, "GAME OVER");
  }

  p.restore();
}


View::View(QWidget* parent) : QWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(480, 360);

  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &View::onTick);
  timer_->start(32);
}


View::~View() {
  if (timer_) timer_->stop();
}


void View::onTick() {
  if (isGameOver()) {
    quit_pending_ = true;
  }
  update();
}


static bool mapKeyToAction(int key, UserAction_t& out) {
  switch (key) {
    case Qt::Key_Left:
      out = Left;
      return true;
    case Qt::Key_Right:
      out = Right;
      return true;
    case Qt::Key_Up:
      out = Up;
      return true;
    case Qt::Key_Down:
      out = Down;
      return true;
    case Qt::Key_Space:
      out = Action;
      return true;
    case Qt::Key_P:
      out = Pause;
      return true;
    case Qt::Key_Q:
      out = Terminate;
      return true;
    case Qt::Key_Escape:
      out = Terminate;
      return true;
    default:
      return false;
  }
}

void View::keyPressEvent(QKeyEvent* ev) {
  if (ev->isAutoRepeat()) {
    ev->accept();
    return;
  }
  UserAction_t a;
  if (mapKeyToAction(ev->key(), a)) {
    userInput(a, true);
    if (a == Terminate) {
      qApp->quit();
      return;
    }
    ev->accept();
    return;
  }
  QWidget::keyPressEvent(ev);
}

void View::keyReleaseEvent(QKeyEvent* ev) {
  if (ev->isAutoRepeat()) {
    ev->accept();
    return;
  }
  if (ev->key() == Qt::Key_Down) {
    userInput(Down, false);
    ev->accept();
    return;
  }
  QWidget::keyReleaseEvent(ev);
}
