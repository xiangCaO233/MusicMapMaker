#include "TimingEditor.h"

// 编辑操作栈
std::stack<TimingEditOperation> TimingEditor::operation_stack;
// 撤回操作栈
std::stack<TimingEditOperation> TimingEditor::undo_stack;

// 构造TimingEditor
TimingEditor::TimingEditor(MapEditor* meditor_ref) : editor_ref(meditor_ref) {}
// 析构TimingEditor
TimingEditor::~TimingEditor() {}

// 鼠标按下事件-传递
void TimingEditor::mouse_pressed(QMouseEvent* e) {}

// 鼠标拖动事件-传递
void TimingEditor::mouse_dragged(QMouseEvent* e) {}

// 撤销
void TimingEditor::undo() {}

// 重做
void TimingEditor::redo() {}
