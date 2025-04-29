#ifndef MINFO_H
#define MINFO_H

#include <QWidget>

class MMetas;

namespace Ui {
class MInfo;
}

class MInfo : public QWidget {
  Q_OBJECT

 public:
  explicit MInfo(QWidget *parent = nullptr);
  ~MInfo();
  MMetas *mmetas;

 private:
  Ui::MInfo *ui;
};

#endif  // MINFO_H
