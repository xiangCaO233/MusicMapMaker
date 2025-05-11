#include "IVMObjectEditor.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/Note.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "colorful-log.h"
#include "editor/edit/HitObjectEditor.h"
#include "editor/edit/eventhandler/mousepress/ivm/IVMCreateComplexHandler.h"
#include "editor/edit/eventhandler/mousepress/ivm/IVMEditComplexHandler.h"
#include "editor/edit/eventhandler/mousepress/ivm/IVMPlaceNoteHandler.h"
#include "editor/edit/eventhandler/mousepress/ivm/IVMSelectHandler.h"
#include "mmm/hitobject/Note/rm/Slide.h"

// 构造IVMObjectEditor
IVMObjectEditor::IVMObjectEditor(MapEditor* meditor_ref)
    : HitObjectEditor(meditor_ref) {
    // 责任链式初始化鼠标按下事件处理器
    // 先处理选中
    mpress_handler = std::make_shared<IVMSelectHandler>();

    // 然后处理放置物件
    auto place_handler = std::make_shared<IVMPlaceNoteHandler>();
    mpress_handler->set_next_handler(place_handler);

    // 然后处理组合键的创建
    auto compcreate_handler = std::make_shared<IVMCreateComplexHandler>();
    place_handler->set_next_handler(compcreate_handler);

    // 然后处理组合键的编辑
    auto compedit_handler = std::make_shared<IVMEditComplexHandler>();
    compcreate_handler->set_next_handler(compedit_handler);
}

// 析构IVMObjectEditor
IVMObjectEditor::~IVMObjectEditor() = default;

// 结束编辑
void IVMObjectEditor::end_edit() {
    // 生成操作并执行到map
    // 生成编辑操作
    ObjEditOperation operation;
    operation.src_objects = editing_src_objects;
    editing_src_objects.clear();

    operation.des_objects = editing_temp_objects;

    // 对map执行操作
    editor_ref->canvas_ref->working_map->execute_edit_operation(operation);

    // 入操作栈
    operation_stack.emplace(operation);
    // 清理正在编辑的物件
    editing_temp_objects.clear();
    current_edit_complex = nullptr;
    current_edit_object = nullptr;
    // 清理快照
    info_shortcuts.clear();
}

// 鼠标按下事件-传递
void IVMObjectEditor::mouse_pressed(QMouseEvent* e) {
    // 抄抄ivm编辑逻辑
    // 鼠标按钮
    auto button = e->button();
    auto handle_res = mpress_handler->handle(
        this, e, nearest_divisor_time(), editor_ref->cstatus.mouse_pos_orbit);
    if (handle_res) {
        XINFO("鼠标按下事件已被处理");
    } else {
        XWARN("鼠标按下事件已忽略");
    }
}

// 更新正编辑的当前组合键
void IVMObjectEditor::update_current_comp() {
    // 将当前编辑缓存的全部物件加入组合键子键中
    current_edit_complex->child_notes.clear();
    for (const auto& obj : editing_temp_objects) {
        auto temp_note = std::dynamic_pointer_cast<Note>(obj);
        if (!temp_note) continue;
        current_edit_complex->child_notes.emplace_back(temp_note);
    }
}

void IVMObjectEditor::mouse_released(QMouseEvent* e) {
    // 鼠标按钮
    auto button = e->button();
    switch (button) {
        case Qt::LeftButton: {
            // 释放的是左键
            //
            //
            // 若为面条编辑模式-
            // 应用更改面条的结果到编辑缓存
            //
            // 若为单键编辑模式-
            // 直接应用编辑缓存的修改到map
            if (!long_note_edit_mode) {
                end_edit();

                XINFO(QString("编辑结束生成物件操作").toStdString());
            } else {
                // 编辑面条模式并非结束
            }
            break;
        }
        case Qt::RightButton: {
            // 释放的是右键
            // 若为面条编辑模式则面条编辑结束
            // 否则删除鼠标悬浮位置的物件
            if (!long_note_edit_mode) {
            } else {
                // 结束面条编辑
                end_edit();
                long_note_edit_mode = false;
                XINFO("结束编辑面条");
            }
        }
    }
}

// 鼠标拖动事件-传递
void IVMObjectEditor::mouse_dragged(QMouseEvent* e) {
    auto time = nearest_divisor_time();
    if (!(std::abs(time - (-1.0)) < 1e-16)) {
        // 根据编辑模式修改所有编辑缓存物件的
        // note
        // 当前编辑物件的相对位置和时间变化值
        // hold
        // 或持续时间
        // slide
        // 或滑动轨道数
        // 计算相对变化
        auto note = std::dynamic_pointer_cast<Note>(current_edit_object);
        // 拖动单键
        // 相对移动的时间和轨道
        auto rtime = time - note->timestamp;
        auto rorbit = editor_ref->cstatus.mouse_pos_orbit - note->orbit;

        // 将编辑缓存中的所有可转化为Note的物件应用此变化
        // 同时遍历缓存和快照-
        auto it = editing_temp_objects.begin();
        auto info_it = info_shortcuts.begin();
        while (it != editing_temp_objects.end() &&
               info_it != info_shortcuts.end()) {
            auto tempnote = std::dynamic_pointer_cast<Note>(*it);
            auto info = std::dynamic_pointer_cast<Note>(*info_it);
            if (tempnote) {
                auto srctime = tempnote->timestamp;
                auto srco = tempnote->orbit;
                tempnote->timestamp = info->timestamp + rtime;
                tempnote->orbit = info->orbit + rorbit;
                // TODO(xiang 2025-05-08):
                // 面条和滑键需要同时更新面尾和滑尾-组合键的时间戳也需要更新
                switch (tempnote->note_type) {
                    case NoteType::HOLD: {
                        auto hold = std::static_pointer_cast<Hold>(tempnote);
                        hold->hold_end_reference->timestamp =
                            tempnote->timestamp + hold->hold_time;
                        break;
                    }
                    case NoteType::SLIDE: {
                        auto slide = std::static_pointer_cast<Slide>(tempnote);
                        slide->slide_end_reference->timestamp =
                            slide->timestamp;
                        slide->slide_end_reference->endorbit =
                            slide->orbit + slide->slide_parameter;
                        break;
                    }
                }
                XINFO(QString("移动物件t[%1]->t[%2]  o[%3]->o[%4]")
                          .arg(srctime)
                          .arg(tempnote->timestamp)
                          .arg(srco)
                          .arg(tempnote->orbit)
                          .toStdString());
            }
            ++it;
            ++info_it;
        }
        // 已在map编辑器过滤
        // switch (editor_ref->edit_mode) {
        //     // 编辑物件的两个模式-拖动事件
        //   case MouseEditMode::PLACE_LONGNOTE:
        //   case MouseEditMode::PLACE_NOTE: {
        //     break;
        //   }
        // }
    }
}
