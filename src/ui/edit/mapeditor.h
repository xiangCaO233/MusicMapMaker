#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QWidget>

#include "HideableToolWindow.hpp"

namespace Ui {
class MapEditor;
}

class MapCanvas;

class MapEditor : public HideableToolWindow {
    Q_OBJECT

   public:
    explicit MapEditor(QWidget *parent = nullptr);
    ~MapEditor() override;

    MapCanvas *canvas() const;

   private:
    Ui::MapEditor *ui;
    // 更新标题后缀
    void update_title_suffix(const QString &suffix);
};

#endif  // MAPEDITOR_H
