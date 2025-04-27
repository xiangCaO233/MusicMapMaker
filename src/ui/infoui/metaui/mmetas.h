#ifndef MMETAS_H
#define MMETAS_H

#include <QWidget>

namespace Ui {
class MMetas;
}

class MMetas : public QWidget {
  Q_OBJECT

 public:
  explicit MMetas(QWidget *parent = nullptr);
  ~MMetas();

 private:
  Ui::MMetas *ui;
};

#endif  // MMETAS_H
