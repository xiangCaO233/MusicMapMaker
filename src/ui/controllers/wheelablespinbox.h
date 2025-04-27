#ifndef M_WHHELABLE_SPINBOX_H
#define M_WHHELABLE_SPINBOX_H

#include <qspinbox.h>
#include <qwidget.h>

#include <QLineEdit>

class WheelableSpinbox : public QDoubleSpinBox {
 public:
  // 构造WheelableSpinbox
  WheelableSpinbox(QWidget* parent) : QDoubleSpinBox(parent) {}

  // 析构WheelableSpinbox
  ~WheelableSpinbox() override = default;

 protected:
  void wheelEvent(QWheelEvent* event) override {
    QDoubleSpinBox::wheelEvent(event);  // 先正常处理滚轮事件
    setFocus();                         // 获取焦点
    lineEdit()->selectAll();            // 选中文本（模拟编辑状态）
  }
};

#endif  // M_WHHELABLE_SPINBOX_H
