#ifndef M_IVMSELECTHANDLER_H
#define M_IVMSELECTHANDLER_H

#include "../IMousePressEventHandler.h"

class IVMSelectHandler : public IMousePressEventHandler {
 public:
  // 构造IVMSelectHandler
  IVMSelectHandler();
  // 析构IVMSelectHandler
  ~IVMSelectHandler() override;

  // 处理事件
  bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
              double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMSELECTHANDLER_H
