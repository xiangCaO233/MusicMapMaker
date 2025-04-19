#ifndef TIMECONTROLLER_H
#define TIMECONTROLLER_H

#include <QWidget>

namespace Ui {
class TimeController;
}

class TimeController : public QWidget {
  Q_OBJECT

 public:
  explicit TimeController(QWidget *parent = nullptr);
  ~TimeController();

 private:
  Ui::TimeController *ui;
};

#endif  // TIMECONTROLLER_H
