#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QWidget>

namespace Ui {
class MapEditor;
}

class MapCanvas;

class MapEditor : public QWidget {
    Q_OBJECT

   public:
    explicit MapEditor(QWidget *parent = nullptr);
    ~MapEditor() override;

    MapCanvas *canvas() const;

   private:
    Ui::MapEditor *ui;
};

#endif  // MAPEDITOR_H
