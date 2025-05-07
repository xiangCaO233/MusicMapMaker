#include "HitObjectEditor.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"

// 编辑操作栈
std::stack<ObjEditOperation> HitObjectEditor::operation_stack;
// 撤回操作栈
std::stack<ObjEditOperation> HitObjectEditor::undo_stack;

// 正在编辑的原物件
std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
    HitObjectEditor::editing_src_objects;

// 正在编辑的缓存物件
std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
    HitObjectEditor::editing_temp_objects;

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
