#include <canvas/GLCanvas.hpp>

void GLCanvas::paintEvent(QPaintEvent *event) {
    QOpenGLWindow::paintEvent(event);
}

void GLCanvas::resizeEvent(QResizeEvent *event) {
    QOpenGLWindow::resizeEvent(event);
}
