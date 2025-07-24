#include <ScalableImageLabel.h>

ScalableImageLabel::ScalableImageLabel(QWidget *parent) : QLabel(parent) {
    // 1. 设置正确的尺寸策略，使其可以在布局中自由缩放
    this->setMinimumSize(1, 1);
    this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // 2. 确保 setScaledContents 是关闭的，因为我们将手动处理缩放
    this->setScaledContents(false);
}

void ScalableImageLabel::setImage(const QPixmap &pixmap) {
    // 存储原始 pixmap
    m_originalPixmap = pixmap;

    // 立即更新一次显示
    updateScaledPixmap();
}

void ScalableImageLabel::clear() {
    // 调用 QLabel 原有的 clear 方法
    QLabel::clear();
    // 清空 pixmap
    m_originalPixmap = QPixmap();
}

void ScalableImageLabel::resizeEvent(QResizeEvent *event) {
    // 当控件大小改变时，调用更新函数
    updateScaledPixmap();

    // 调用基类的实现
    QLabel::resizeEvent(event);
}

void ScalableImageLabel::updateScaledPixmap() {
    // 如果没有原始图片，就不用做任何事
    if (m_originalPixmap.isNull()) {
        return;
    }
    // 调用 QLabel::setPixmap() 设置缩放后的图片
    QLabel::setPixmap(m_originalPixmap.scaled(
        // 目标大小为当前 Label 的大小
        this->size(),
        // 保持宽高比
        Qt::KeepAspectRatio,
        // 使用平滑的高质量缩放
        Qt::SmoothTransformation));
}
