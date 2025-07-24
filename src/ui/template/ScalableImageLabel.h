#ifndef MMM_SCALABLEIMAGELABEL_H
#define MMM_SCALABLEIMAGELABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <cstdint>

class ScalableImageLabel : public QLabel {
    Q_OBJECT

   public:
    explicit ScalableImageLabel(QWidget *parent = nullptr);

    // 设置图片
    void setImage(const QPixmap &pixmap);
    void setImage(uint8_t *image_data);

    // hide original clear func
    void clear();

   protected:
    // 重写 resizeEvent 来处理缩放
    void resizeEvent(QResizeEvent *event) override;

   private:
    // 原始未缩放的 pixmap
    QPixmap m_originalPixmap;

    // 一个用于更新显示的辅助函数
    void updateScaledPixmap();
};

#endif  // MMM_SCALABLEIMAGELABEL_H
