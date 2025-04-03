#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "GLCanvas.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  GLCanvas *glCanvas;
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

 private:
  Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
