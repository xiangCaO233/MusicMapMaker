#include <audio/graphic/AudioGraphicWidget.h>

#include <QWheelEvent>

void AudioGraphicWidget::showEvent(QShowEvent* event) {
    QOpenGLWidget::showEvent(event);
}

void AudioGraphicWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        lastMousePos = event->pos();
        isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void AudioGraphicWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton && isPanning) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

void AudioGraphicWidget::mouseMoveEvent(QMouseEvent* event) {
    if (isPanning) {
        const QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();

        // 将像素偏移转换为时间偏移
        auto time_delta = pixelsToTime(delta.x());

        // 更新视图起始时间 (注意是减去，因为向右拖动(delta.x >
        // 0)意味着时间变小)
        auto new_start_time = viewStartTime - time_delta;

        // 边界检查
        if (new_start_time < std::chrono::nanoseconds(0)) {
            new_start_time = std::chrono::nanoseconds(0);
        }

        // 检查是否超出音轨末尾
        if (const auto total_track_time =
                framesToTime(audio_track->num_frames(),
                             audio_track->get_media_info().format.samplerate);
            new_start_time + visibleTimeRange > total_track_time) {
            new_start_time = total_track_time - visibleTimeRange;
            if (new_start_time < std::chrono::nanoseconds(0)) {
                new_start_time = std::chrono::nanoseconds(0);
            }
        }

        viewStartTime = new_start_time;
        update();
        event->accept();
    }
}

void AudioGraphicWidget::wheelEvent(QWheelEvent* event) {
    const float zoom_factor = 1.15f;
    const QPoint angle_delta = event->angleDelta();

    // 获取鼠标在控件内的位置和时间
    const double mouse_x = event->position().x();
    const auto time_at_cursor_before_zoom =
        viewStartTime + pixelsToTime(mouse_x);

    // 计算新的可见时间范围
    std::chrono::nanoseconds new_visible_range;
    if (angle_delta.y() > 0) {  // 向上滚轮，放大
        new_visible_range =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                visibleTimeRange / zoom_factor);
    } else {  // 向下滚轮，缩小
        new_visible_range =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                visibleTimeRange * zoom_factor);
    }

    // 边界检查
    const auto min_range = std::chrono::nanoseconds(10000000);
    const auto max_range =
        framesToTime(audio_track->num_frames(),
                     audio_track->get_media_info().format.samplerate);
    new_visible_range =
        std::max(min_range, std::min(new_visible_range, max_range));

    // 计算新的视图起始时间，以保持鼠标指向的时间点不变
    const double mouse_x_ratio = mouse_x / width();
    viewStartTime = time_at_cursor_before_zoom -
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        new_visible_range * mouse_x_ratio);

    // 更新可见范围并进行边界检查
    visibleTimeRange = new_visible_range;
    if (viewStartTime < std::chrono::nanoseconds(0)) {
        viewStartTime = std::chrono::nanoseconds(0);
    }
    update();
    event->accept();
}
