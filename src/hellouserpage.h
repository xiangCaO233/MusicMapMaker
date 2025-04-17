#ifndef HELLOUSERPAGE_H
#define HELLOUSERPAGE_H

#include <QWidget>

namespace Ui {
class HelloUserPage;
}

class HelloUserPage : public QWidget {
  Q_OBJECT

 public:
  explicit HelloUserPage(QWidget *parent = nullptr);
  ~HelloUserPage();

 private:
  Ui::HelloUserPage *ui;
};

#endif  // HELLOUSERPAGE_H
