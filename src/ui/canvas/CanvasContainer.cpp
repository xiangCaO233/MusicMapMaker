#include <CanvasContainer.h>

#include <QVBoxLayout>

// 构造CanvasContainer
CanvasContainer::CanvasContainer(QWidget* parent) : QWidget(parent) {
    // 创建 QOpenGLWindow
    canvas.reset(new GLCanvas());

    // 将 QOpenGLWindow 嵌入到 QWidget
    container = QWidget::createWindowContainer(canvas.data(), this);

    // 设置布局
    auto layout = new QVBoxLayout(this);
    layout->addWidget(container);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
}

// 析构CanvasContainer
CanvasContainer::~CanvasContainer() { qDebug() << "Canvas deleted"; }
