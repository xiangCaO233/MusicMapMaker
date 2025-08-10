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

    // 获取画布实例
    MapCanvas *canvas() const;

   protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

   private:
    Ui::MapEditor *ui;
};

#endif  // MAPEDITOR_H
