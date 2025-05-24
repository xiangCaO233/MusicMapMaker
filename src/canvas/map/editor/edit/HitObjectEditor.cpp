#include "HitObjectEditor.h"

#include <memory>

#include "../../../mmm/MapWorkProject.h"
#include "../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../../util/mutil.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "colorful-log.h"

// 构造HitObjectEditor
HitObjectEditor::HitObjectEditor(MapEditor* meditor_ref)
    : editor_ref(meditor_ref) {}
// 析构HitObjectEditor
HitObjectEditor::~HitObjectEditor() {}

// 撤销
void HitObjectEditor::undo() {
    if (operation_stack.empty()) return;
    auto& operation = operation_stack.top();
    auto reverse = operation.reverse_clone();
    undo_stack.push(reverse);
    // 撤回上一操作
    if (editor_ref->canvas_ref->working_map) {
        editor_ref->canvas_ref->working_map->execute_edit_operation(reverse);
        // 出栈
        operation_stack.pop();
    } else {
        // 没了-清空全部栈
        while (!operation_stack.empty()) {
            operation_stack.pop();
        }
        while (!undo_stack.empty()) {
            undo_stack.pop();
        }
    }
}

// 重做
void HitObjectEditor::redo() {
    if (undo_stack.empty()) return;
    auto& uoperation = undo_stack.top();
    auto reverse = uoperation.reverse_clone();
    operation_stack.push(reverse);
    // 重做撤回栈顶的操作
    if (editor_ref->canvas_ref->working_map) {
        editor_ref->canvas_ref->working_map->execute_edit_operation(reverse);
        // 出栈
        undo_stack.pop();
    } else {
        // 没了-清空全部栈
        while (!operation_stack.empty()) {
            operation_stack.pop();
        }
        while (!undo_stack.empty()) {
            undo_stack.pop();
        }
    }
}

// 复制
void HitObjectEditor::copy() {
    // 获取选中物件-不清除选中物件-只清除选中框
    editing_src_objects.clear();
    clipboard.clear();
    for (const auto& o : editor_ref->ebuffer.selected_hitobjects) {
        clipboard.insert(o);
        auto note = std::dynamic_pointer_cast<Note>(o);
        if (note) {
            if (note->compinfo != ComplexInfo::NONE) {
                // 拷贝到了组合键中的子键-添加组合键引用
                auto temp_comp = std::shared_ptr<ComplexNote>(
                    note->parent_reference, [](ComplexNote*) {});
                if (!temp_comp) continue;
                auto com_it = clipboard.find(temp_comp);
                if (com_it == clipboard.end()) {
                    clipboard.insert(
                        std::shared_ptr<ComplexNote>(temp_comp->clone()));
                }
            }
        }
    }
    editor_ref->ebuffer.select_bound.setWidth(0);
    editor_ref->ebuffer.select_bound.setHeight(0);
    editor_ref->ebuffer.select_bound_locate_points = nullptr;
    XINFO("选中物件已复制到剪切板");
}

// 剪切
void HitObjectEditor::cut() {
    // 获取选中物件-清除选中物件-进入选中状态(加入editing_src_objects和editing_temp_objects)
    editing_src_objects.clear();
    editing_temp_objects.clear();
    clipboard.clear();
    editor_ref->ebuffer.select_bound.setWidth(0);
    editor_ref->ebuffer.select_bound.setHeight(0);
    editor_ref->ebuffer.select_bound_locate_points = nullptr;

    for (const auto& o : editor_ref->ebuffer.selected_hitobjects) {
        editing_src_objects.insert(o);
        editing_temp_objects.insert(std::shared_ptr<HitObject>(o->clone()));
        clipboard.insert(o);
        auto note = std::dynamic_pointer_cast<Note>(o);
        if (note) {
            if (note->compinfo != ComplexInfo::NONE) {
                // 拷贝到了组合键中的子键-添加组合键引用
                auto temp_comp = std::shared_ptr<ComplexNote>(
                    note->parent_reference, [](ComplexNote*) {});
                if (!temp_comp) continue;
                auto com_it = clipboard.find(temp_comp);
                if (com_it == clipboard.end()) {
                    clipboard.insert(
                        std::shared_ptr<ComplexNote>(temp_comp->clone()));
                }
            }
        }
    }
    editor_ref->ebuffer.selected_hitobjects.clear();
    XINFO("选中物件已剪切到剪切板");
}

// 粘贴
void HitObjectEditor::paste() {}

// 鼠标最近的分拍线的时间
double HitObjectEditor::nearest_divisor_time() {
    // 查询当前鼠标位置最近的拍-在最近的分拍线放置
    auto beats = editor_ref->canvas_ref->working_map->query_beat_before_time(
        editor_ref->cstatus.mouse_pos_time);

    // 查到拍则获取最近的分拍线
    int32_t min_deviation_time = 2147483647;
    int32_t res_time;
    for (const auto& beat : beats) {
        for (int i = 0; i < beat->divisors; ++i) {
            auto div_time = beat->start_timestamp +
                            i * (beat->end_timestamp - beat->start_timestamp) /
                                beat->divisors;
            auto deviation_time =
                std::abs(editor_ref->cstatus.mouse_pos_time - div_time);
            if (deviation_time < min_deviation_time) {
                min_deviation_time = deviation_time;
                res_time = div_time;
            }
        }
    }
    return beats.empty() ? -1.0 : res_time;
}

// 分析组合键拆分
void HitObjectEditor::analyze_src_object() {}

// 鼠标按下事件-传递
void HitObjectEditor::mouse_pressed(QMouseEvent* e) {}

// 鼠标释放事件-传递
void HitObjectEditor::mouse_released(QMouseEvent* e) {}

// 鼠标拖动事件-传递
void HitObjectEditor::mouse_dragged(QMouseEvent* e) {}
