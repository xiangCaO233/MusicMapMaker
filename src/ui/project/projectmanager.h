#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QWidget>

#include "template/HideableToolWindow.hpp"

namespace Ui {
class ProjectManager;
}

class ProjectManager : public HideableToolWindow {
    Q_OBJECT

   public:
    explicit ProjectManager(QWidget *parent = nullptr);
    ~ProjectManager();

   private:
    Ui::ProjectManager *ui;
};

#endif  // PROJECTMANAGER_H
