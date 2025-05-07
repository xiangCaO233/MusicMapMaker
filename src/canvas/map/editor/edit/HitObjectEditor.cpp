#include "HitObjectEditor.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"

// 构造HitObjectEditor
HitObjectEditor::HitObjectEditor(MapEditor* meditor_ref)
    : editor_ref(meditor_ref) {}
// 析构HitObjectEditor
HitObjectEditor::~HitObjectEditor() {}

// 撤销
void HitObjectEditor::undo() {}

// 重做
void HitObjectEditor::redo() {}

// 鼠标按下事件-传递
void HitObjectEditor::mouse_pressed(QMouseEvent* e) {}
