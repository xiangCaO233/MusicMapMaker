#include "IVMTimingEditor.h"

#include "editor/edit/TimingEditor.h"

// 构造IVMTimingEditor
IVMTimingEditor::IVMTimingEditor(MapEditor* meditor_ref)
    : TimingEditor(meditor_ref) {}

// 析构IVMTimingEditor
IVMTimingEditor::~IVMTimingEditor() {}

// 结束编辑-生成可撤回操作入栈
void IVMTimingEditor::end_edit() {}

// 鼠标按下事件-传递
void IVMTimingEditor::mouse_pressed(QMouseEvent* e) {}
// 鼠标释放事件-传递
void IVMTimingEditor::mouse_released(QMouseEvent* e) {}
// 鼠标拖动事件-传递
void IVMTimingEditor::mouse_dragged(QMouseEvent* e) {}
