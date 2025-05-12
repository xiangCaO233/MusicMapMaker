#ifndef M_IVMDRAGADJUSTOBJECTHANDLER_H
#define M_IVMDRAGADJUSTOBJECTHANDLER_H

#include "../IMouseDragEventHandler.h"

class IVMDragAdjustObjectHandler : public IMouseDragEventHandler {
   public:
    // 构造IVMDragAdjustObjectHandler
    IVMDragAdjustObjectHandler();

    // 析构IVMDragAdjustObjectHandler
    ~IVMDragAdjustObjectHandler() override;

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMDRAGADJUSTOBJECTHANDLER_H
