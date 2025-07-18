#include <QScopedPointer>
#include <QWidget>
#include <canvas/GLCanvas.hpp>

class CanvasContainer : public QWidget {
   public:
    // 构造CanvasContainer
    explicit CanvasContainer(QWidget* parent = nullptr);

    // 析构CanvasContainer
    ~CanvasContainer() override;

    // 画布本体
    QScopedPointer<GLCanvas> canvas;

    // 用于嵌入 QOpenGLWindow
    QWidget* container;
};
