#ifndef TIMINGMANAGER_H
#define TIMINGMANAGER_H

#include <QWidget>

namespace Ui {
class TimingEditor;
}

class TimingManager : public QWidget {
    Q_OBJECT

   public:
    explicit TimingManager(QWidget *parent = nullptr);
    ~TimingManager();

   private:
    Ui::TimingEditor *ui;
};

#endif  // TIMINGMANAGER_H
