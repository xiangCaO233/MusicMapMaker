#include <canvas/GLCanvas.hpp>

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

void GLCanvas::resizeEvent(QResizeEvent *event) {
    QOpenGLWindow::resizeEvent(event);
}
