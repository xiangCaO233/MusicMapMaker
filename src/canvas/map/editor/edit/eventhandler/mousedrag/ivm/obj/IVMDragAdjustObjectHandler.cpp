#include "IVMDragAdjustObjectHandler.h"

#include "../../../../../../MapWorkspaceCanvas.h"
#include "../../../../../MapEditor.h"
#include "../../../../IVMObjectEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "colorful-log.h"

// 构造IVMDragAdjustObjectHandler
IVMDragAdjustObjectHandler::IVMDragAdjustObjectHandler() {}

// 析构IVMDragAdjustObjectHandler
IVMDragAdjustObjectHandler::~IVMDragAdjustObjectHandler() = default;

// 处理事件
bool IVMDragAdjustObjectHandler::handle(HitObjectEditor* oeditor_context,
                                        QMouseEvent* e, double mouse_time,
                                        double mouse_orbit) {
    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
    switch (ivmobjecteditor->hover_object_info_shortcut->part) {
        case HoverPart::HOLD_END: {
            // 拖动某个面条的尾部
            // hold
            // TODO(xiang 2025-05-12): 调整所有面条的持续时间
            return true;
        }
        case HoverPart::SLIDE_END: {
            // 拖动某个滑键的尾部
            // slide
            // TODO(xiang 2025-05-12):调整所有滑键的滑动轨道数
            return true;
        }
        default:
            return false;
    }
}
