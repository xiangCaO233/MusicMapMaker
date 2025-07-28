#include <audio/graphic/AudioGraphicWidget.h>

#include <QWheelEvent>

void AudioGraphicWidget::showEvent(QShowEvent *event) {
    QOpenGLWidget::showEvent(event);
}
void AudioGraphicWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        isPanning = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::LeftButton) {
        // 跳转
    }
}

void AudioGraphicWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isPanning) {
        // 拖拽
        int dx = event->pos().x() - lastMousePos.x();
        lastMousePos = event->pos();

        auto framesPerPixel = static_cast<double>(visibleFrameRange) / width();
        auto frameDelta = static_cast<qint64>(dx * framesPerPixel);

        viewStartFrame -= frameDelta;

        // 边界检查
        viewStartFrame = qMax(0LL, viewStartFrame);
        if (audio_track) {
            viewStartFrame =
                qMin(qint64(audio_track->num_frames() - visibleFrameRange),
                     viewStartFrame);
        }
        update();
    } else {
        // 移动-显示当前鼠标位置对应的时间戳
    }
}

void AudioGraphicWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton && isPanning) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void AudioGraphicWidget::wheelEvent(QWheelEvent *event) {
    if (!audio_track) return;

    double zoomFactor = (event->angleDelta().y() > 0) ? 0.8 : 1.25;
    double framesPerPixel = static_cast<double>(visibleFrameRange) / width();
    qint64 mouseFrame =
        viewStartFrame +
        static_cast<qint64>(event->position().x() * framesPerPixel);

    size_t newRange = visibleFrameRange * zoomFactor;
    // 限制范围
    newRange = qMax((size_t)width(), newRange);
    newRange = qMin(audio_track->num_frames(), newRange);
    visibleFrameRange = newRange;

    // 以鼠标为中心缩放
    viewStartFrame =
        mouseFrame - static_cast<qint64>(event->position().x() *
                                         visibleFrameRange / width());
    viewStartFrame = qMax(0LL, viewStartFrame);
    viewStartFrame = qMin(qint64(audio_track->num_frames() - visibleFrameRange),
                          viewStartFrame);
}
