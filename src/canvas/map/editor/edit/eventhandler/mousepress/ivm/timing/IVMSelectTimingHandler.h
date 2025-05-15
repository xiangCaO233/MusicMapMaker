#ifndef M_IVMSELECTTIMINGHANDLER_H
#define M_IVMSELECTTIMINGHANDLER_H

#include "../../IMousePressEventHandler.h"

class IVMSelectTimingHandler : public IMousePressEventHandler {
   public:
    // 构造IVMSelectTimingHandler
    IVMSelectTimingHandler();
    // 析构IVMSelectTimingHandler
    ~IVMSelectTimingHandler() override;

    // 处理事件
    bool handle(TimingEditor* oeditor_context, QMouseEvent* e,
                double mouse_time) override;
};

#endif  // M_IVMSELECTTIMINGHANDLER_H
