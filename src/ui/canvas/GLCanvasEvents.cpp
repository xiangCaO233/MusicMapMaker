#include <QMouseEvent>
#include <canvas/GLCanvas.hpp>
#include <layer/LayerManager.hpp>

void GLCanvas::paintEvent(QPaintEvent *event) {
    QOpenGLWindow::paintEvent(event);
    static long long lasttime = 0;
    auto time =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    auto atime = time - lasttime;
    actual_update_time = double(atime) / 1000.0;
    lasttime = time;
}

void GLCanvas::resizeEvent(QResizeEvent *e) {
    QOpenGLWindow::resizeEvent(e);
    canvas_info->baseInfo.canvasSize = e->size();
    render_dataloop->layermanager()->updateInfoForLayers(canvas_info.get());
}

void GLCanvas::keyPressEvent(QKeyEvent *e) {}

void GLCanvas::keyReleaseEvent(QKeyEvent *e) {}

void GLCanvas::mouseMoveEvent(QMouseEvent *e) {
    canvas_info->realTimeInfo.mousePos = e->pos();
    render_dataloop->layermanager()->updateInfoForLayers(canvas_info.get());
}

void GLCanvas::mousePressEvent(QMouseEvent *e) {
    canvas_info->realTimeInfo.buttons.insert(e->button());
    render_dataloop->layermanager()->updateInfoForLayers(canvas_info.get());
}

void GLCanvas::mouseReleaseEvent(QMouseEvent *e) {
    canvas_info->realTimeInfo.buttons.erase(e->button());
    render_dataloop->layermanager()->updateInfoForLayers(canvas_info.get());
}

void GLCanvas::closeEvent(QCloseEvent *e) { render_dataloop->stop(); }
