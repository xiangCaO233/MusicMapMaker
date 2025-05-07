#include "MMMObjectEditor.h"

#include "editor/edit/HitObjectEditor.h"

// 构造MMMObjectEditor
MMMObjectEditor::MMMObjectEditor(MapEditor* meditor_ref)
    : HitObjectEditor(meditor_ref) {}

// 析构MMMObjectEditor
MMMObjectEditor::~MMMObjectEditor() = default;

// 鼠标按下事件-传递
void MMMObjectEditor::mouse_pressed(QMouseEvent* e) {}

// 鼠标拖动事件-传递
void MMMObjectEditor::mouse_dragged(QMouseEvent* e) {}
