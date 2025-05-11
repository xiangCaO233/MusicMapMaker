#ifndef M_IVMCREATE_COMPLEX_HANDLER_H
#define M_IVMCREATE_COMPLEX_HANDLER_H

#include "../IMousePressEventHandler.h"

// ivm创建组合键处理器
class IVMCreateComplexHandler : public IMousePressEventHandler {
 public:
  // 构造IVMCreateComplexHandler
  IVMCreateComplexHandler();

  // 析构IVMCreateComplexHandler
  ~IVMCreateComplexHandler() override;

  // 处理事件
  bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
              double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMCREATE_COMPLEX_HANDLER_H
