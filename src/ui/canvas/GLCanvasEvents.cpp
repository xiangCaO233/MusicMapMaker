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

    if (m_last_paint_time_us == 0) {
        m_last_paint_time_us = now_us;
        return;  // 第一帧不做处理
    }
    if (!canvas_info->realTimeInfo.pause) {
        // 2. 从原子变量中加载最新的校准点
        double last_audio_time =
            canvas_info->realTimeInfo.last_audio_time_ms.load();
        long long last_sync_point =
            canvas_info->realTimeInfo.last_sync_point_us.load();

        // 3. 计算自上一个“校准点”以来，系统时间过去了多久
        double delta_since_sync_ms = (now_us - last_sync_point) / 1000.0;

        // 4. 推算出当前的“理想”画布时间
        double estimated_time = last_audio_time + delta_since_sync_ms;

        // 5. 平滑地修正画布时间
        // 直接设置: canvas_info->realTimeInfo.current_canvas_time =
        // estimated_time;
        // 这种方式最直接，但如果音频回调有抖动，可能会导致画面微小的跳跃。

        // 更平滑的方式 (低通滤波/lerp)：
        // 让画布时间以一个较快的速度“追赶”理想时间，而不是瞬时跳变。
        double current_time = canvas_info->realTimeInfo.current_canvas_time;
        constexpr float LERP_RATIO = 0.15f;
        // 0.1 是一个平滑系数，可以调整。值越大，追赶速度越快。
        canvas_info->realTimeInfo.current_canvas_time =
            current_time + (estimated_time - current_time) * LERP_RATIO;

        // 如果不需要平滑，可以直接使用下面这行：
        // canvas_info->realTimeInfo.current_canvas_time = estimated_time;
    }

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
