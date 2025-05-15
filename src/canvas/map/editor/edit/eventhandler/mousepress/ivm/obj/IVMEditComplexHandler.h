#ifndef M_IVMEDIT_COMPLEX_HANDLER_H
#define M_IVMEDIT_COMPLEX_HANDLER_H

#include "../../IMousePressEventHandler.h"

class IVMEditComplexHandler : public IMousePressEventHandler {
   public:
    // 构造IVMEditComplexHandler
    IVMEditComplexHandler();
    // 析构IVMEditComplexHandler
    ~IVMEditComplexHandler() override;

    // 处理事件
    bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMEDIT_COMPLEX_HANDLER_H
