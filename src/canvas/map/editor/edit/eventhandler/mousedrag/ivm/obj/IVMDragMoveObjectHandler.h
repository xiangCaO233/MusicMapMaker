#ifndef M_IVMDRAGMOVEOBJECTHANDLER_H
#define M_IVMDRAGMOVEOBJECTHANDLER_H

#include <memory>

#include "../../IMouseDragEventHandler.h"

class IVMObjectEditor;
class Note;

class IVMDragMoveObjectHandler : public IMouseDragEventHandler {
   public:
    // 构造IVMDragMoveObjectHandler
    IVMDragMoveObjectHandler();
    // 析构IVMDragMoveObjectHandler
    ~IVMDragMoveObjectHandler() override;

    // 相对地移动缓存物件
    void move_temp_objectsrelatively(IVMObjectEditor* ivmobjecteditor,
                                     double rtime, int32_t rorbit);

    // 检查物件合法性
    bool check_object(IVMObjectEditor* ivmobjecteditor,
                      std::shared_ptr<Note> note, std::shared_ptr<Note> info,
                      double rtime, int32_t rorbit, bool auto_fix = false);

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMDRAGMOVEOBJECTHANDLER_H
