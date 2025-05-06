#include "TimingEditor.h"

// 构造TimingEditor
TimingEditor::TimingEditor(MapEditor* meditor_ref) : editor_ref(meditor_ref) {}
// 析构TimingEditor
TimingEditor::~TimingEditor() {}

// 鼠标按下事件-传递
void TimingEditor::mouse_pressed(QMouseEvent* e) {}

// 撤销
void TimingEditor::undo() {}

// 重做
void TimingEditor::redo() {}
