#ifndef M_CANVASCONTAINER_H
#define M_CANVASCONTAINER_H

#include <QWidget>

#include "../../GlobalSettings.h"
#include "MapWorkspaceCanvas.h"

class CanvasContainer : public QWidget {
   public:
    // 构造CanvasContainer
    CanvasContainer(QWidget* parent = nullptr);
    // 析构CanvasContainer
    ~CanvasContainer() override;

    // 当前主题
    GlobalTheme current_theme;

    // 画布本体
    QScopedPointer<MapWorkspaceCanvas> canvas;

    // 用于嵌入 QOpenGLWindow
    QWidget* container;

    // 使用主题
    void use_theme(GlobalTheme theme);
};

#endif  // M_CANVASCONTAINER_H
