#ifndef MINFO_H
#define MINFO_H

#include <QWidget>

namespace Ui {
class MInfo;
}

class MInfo : public QWidget {
  Q_OBJECT

 public:
  explicit MInfo(QWidget *parent = nullptr);
  ~MInfo();

 private:
  Ui::MInfo *ui;
};

#endif  // MINFO_H
