#include "IVMSelectTimingHandler.h"

#include "../../../../IVMTimingEditor.h"

// 构造IVMSelectTimingHandler
IVMSelectTimingHandler::IVMSelectTimingHandler() {}

// 析构IVMSelectTimingHandler
IVMSelectTimingHandler::~IVMSelectTimingHandler() = default;

// 处理事件
bool IVMSelectTimingHandler::handle(TimingEditor* teditor_context,
                                    QMouseEvent* e, double mouse_time) {
    auto ivmtimingeditor = static_cast<IVMTimingEditor*>(teditor_context);
    // 若有则选中鼠标悬浮处的timing
    // 无则传递给新建timing事件
    return false;
}
