#ifndef MAPCONFIG_H
#define MAPCONFIG_H

#include <QWidget>

namespace Ui {
class MapConfig;
}

class MapConfig : public QWidget {
    Q_OBJECT

   public:
    explicit MapConfig(QWidget *parent = nullptr);
    ~MapConfig();

   private:
    Ui::MapConfig *ui;
};

#endif  // MAPCONFIG_H
