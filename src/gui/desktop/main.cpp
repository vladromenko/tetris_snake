#include <QApplication>

#include "view.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  View w;
  w.setWindowTitle("BrickGame");
  w.show();
  return app.exec();
}