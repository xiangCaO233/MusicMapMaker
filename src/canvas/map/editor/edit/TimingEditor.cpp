#include "TimingEditor.h"

// 构造TimingEditor
TimingEditor::TimingEditor(MapEditor* meditor_ref) : editor_ref(meditor_ref) {}
// 析构TimingEditor
TimingEditor::~TimingEditor() {}

// 鼠标按下事件-传递
void TimingEditor::mouse_pressed(QMouseEvent* e) {}

// 鼠标释放事件-传递
void TimingEditor::mouse_released(QMouseEvent* e) {}

// 鼠标拖动事件-传递
void TimingEditor::mouse_dragged(QMouseEvent* e) {}

// 撤销
void TimingEditor::undo() {}

// 重做
void TimingEditor::redo() {}

// 复制
void TimingEditor::copy() {}
// 剪切
void TimingEditor::cut() {}
// 粘贴
void TimingEditor::paste() {}
