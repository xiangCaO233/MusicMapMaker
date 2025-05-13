#ifndef NEWMAPGUIDE_H
#define NEWMAPGUIDE_H

#include <QDialog>

namespace Ui {
class NewMapGuide;
}

class NewMapGuide : public QDialog {
    Q_OBJECT

   public:
    explicit NewMapGuide(QWidget *parent = nullptr);
    ~NewMapGuide();

   private:
    Ui::NewMapGuide *ui;
};

#endif  // NEWMAPGUIDE_H
