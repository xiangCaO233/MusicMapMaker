#ifndef M_IVMTIMINGEDITOR_H
#define M_IVMTIMINGEDITOR_H

#include "TimingEditor.h"

class IVMTimingEditor : public TimingEditor {
   public:
    // 构造IVMTimingEditor
    IVMTimingEditor(MapEditor* meditor_ref);
    // 析构IVMTimingEditor
    ~IVMTimingEditor() override;

    // 结束编辑-生成可撤回操作入栈
    void end_edit();

    // 鼠标按下事件-传递
    void mouse_pressed(QMouseEvent* e) override;
    // 鼠标释放事件-传递
    void mouse_released(QMouseEvent* e) override;
    // 鼠标拖动事件-传递
    void mouse_dragged(QMouseEvent* e) override;
};

#endif  // M_IVMTIMINGEDITOR_H
