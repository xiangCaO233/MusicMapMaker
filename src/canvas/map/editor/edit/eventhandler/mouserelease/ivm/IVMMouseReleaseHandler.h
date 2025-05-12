#ifndef M_IVMMOUSEREASEHANDLER_H
#define M_IVMMOUSEREASEHANDLER_H

#include "../IMouseReleaseEventHandler.h"

class IVMMouseReleaseHandler : public IMouseReleaseEventHandler {
   public:
    // 构造IVMMouseReleaseHandler
    IVMMouseReleaseHandler();
    // 析构IVMMouseReleaseHandler
    ~IVMMouseReleaseHandler() override;

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMMOUSEREASEHANDLER_H
