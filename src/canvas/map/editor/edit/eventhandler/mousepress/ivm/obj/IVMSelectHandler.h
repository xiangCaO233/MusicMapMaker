#ifndef M_IVMSELECTHANDLER_H
#define M_IVMSELECTHANDLER_H

#include <memory>

#include "../../IMousePressEventHandler.h"

class HitObject;
class IVMObjectEditor;

class IVMSelectHandler : public IMousePressEventHandler {
   public:
    // 构造IVMSelectHandler
    IVMSelectHandler();
    // 析构IVMSelectHandler
    ~IVMSelectHandler() override;

    // 选中物件
    void select_note(IVMObjectEditor* ivmobjecteditor, HitObject* note);

    // 处理事件
    bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMSELECTHANDLER_H
