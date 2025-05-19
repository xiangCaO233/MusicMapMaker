#include "CanvasContainer.h"

#include <qwidget.h>

#include <QApplication>
#include <QOpenGLWindow>
#include <QVBoxLayout>

#include "colorful-log.h"

// 构造CanvasContainer
CanvasContainer::CanvasContainer(QWidget* parent) : QWidget(parent) {
    // 创建 QOpenGLWindow
    canvas.reset(new MapWorkspaceCanvas());

    // 将 QOpenGLWindow 嵌入到 QWidget
    container = QWidget::createWindowContainer(canvas.data(), this);
    // 设置布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(container);
    layout->setContentsMargins(0, 0, 0, 0);
}

// 析构CanvasContainer
CanvasContainer::~CanvasContainer() = default;

// 使用主题
void CanvasContainer::use_theme(GlobalTheme theme) { canvas->use_theme(theme); }
