#include <QMouseEvent>
#include <canvas/GLCanvas.hpp>
#include <layer/LayerManager.hpp>

void GLCanvas::paintEvent(QPaintEvent *event) {
    QOpenGLWindow::paintEvent(event);
    // 1. 获取当前的系统高精度时间
    long long now_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    last_update_time_us = now_us - m_last_paint_time_us;
    m_last_paint_time_us = now_us;
}

void GLCanvas::resizeEvent(QResizeEvent *e) {
    QOpenGLWindow::resizeEvent(e);
    canvas_info->baseInfo.canvasSize = e->size();
    update_sharedInfo();
}

void GLCanvas::keyPressEvent(QKeyEvent *e) { QOpenGLWindow::keyPressEvent(e); }

void GLCanvas::keyReleaseEvent(QKeyEvent *e) {
    QOpenGLWindow::keyReleaseEvent(e);
}

void GLCanvas::mouseMoveEvent(QMouseEvent *e) {
    QOpenGLWindow::mouseMoveEvent(e);
    canvas_info->realTimeInfo.mousePos = e->pos();
    update_sharedInfo();
}

void GLCanvas::mousePressEvent(QMouseEvent *e) {
    QOpenGLWindow::mousePressEvent(e);
    canvas_info->realTimeInfo.buttons.insert(e->button());
    update_sharedInfo();
}

void GLCanvas::mouseReleaseEvent(QMouseEvent *e) {
    QOpenGLWindow::mouseReleaseEvent(e);
    canvas_info->realTimeInfo.buttons.erase(e->button());
    update_sharedInfo();
}

void GLCanvas::closeEvent(QCloseEvent *e) { render_dataloop->stop(); }
