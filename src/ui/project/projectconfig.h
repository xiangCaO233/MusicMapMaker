#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QWidget>

namespace Ui {
class ProjectConfig;
}

class ProjectConfig : public QWidget {
    Q_OBJECT

   public:
    explicit ProjectConfig(QWidget *parent = nullptr);
    ~ProjectConfig();

   private:
    Ui::ProjectConfig *ui;
};

#endif  // PROJECTCONFIG_H
