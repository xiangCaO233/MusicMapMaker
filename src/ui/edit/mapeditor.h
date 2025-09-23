#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <GlobalSettings.hpp>
#include <QButtonGroup>
#include <QPushButton>
#include <QWidget>

namespace Ui {
class MapEditor;
}

class MapCanvas;
class Timing;

class MapEditor : public QWidget {
    Q_OBJECT

   public:
    explicit MapEditor(QWidget *parent = nullptr);
    ~MapEditor() override;

    // 获取画布实例
    MapCanvas *canvas() const;

    // 使用主题
    void use_theme(GlobalTheme theme);

   protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

   private slots:
    void on_timeline_effect_button_toggled(bool checked);

    void on_magnet_to_divisor_button_toggled(bool checked);

    void on_scroll_direction_button_toggled(bool checked);

   private:
    Ui::MapEditor *ui;

    // 模式按钮组
    QButtonGroup *modesbuttonGroup;
    QPushButton *hand_mode_button;
    QPushButton *note_mode_button;

    void initializeMenus();
    void initializeToolsMenu();
    void initializeBgMenu();
};

#endif  // MAPEDITOR_H
