#ifndef M_IVMDRAGMOVEOBJECTHANDLER_H
#define M_IVMDRAGMOVEOBJECTHANDLER_H

#include "../../IMouseDragEventHandler.h"

class IVMDragMoveObjectHandler : public IMouseDragEventHandler {
   public:
    // 构造IVMDragMoveObjectHandler
    IVMDragMoveObjectHandler();
    // 析构IVMDragMoveObjectHandler
    ~IVMDragMoveObjectHandler() override;

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMDRAGMOVEOBJECTHANDLER_H
