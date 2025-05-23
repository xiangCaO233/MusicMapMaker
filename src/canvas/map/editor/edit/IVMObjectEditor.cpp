#include "IVMObjectEditor.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/Note.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../../../util/mutil.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "colorful-log.h"
#include "eventhandler/mousedrag/ivm/obj/IVMDragAdjustObjectHandler.h"
#include "eventhandler/mousedrag/ivm/obj/IVMDragMoveObjectHandler.h"
#include "eventhandler/mousepress/ivm/obj/IVMCreateComplexHandler.h"
#include "eventhandler/mousepress/ivm/obj/IVMEditComplexHandler.h"
#include "eventhandler/mousepress/ivm/obj/IVMPlaceNoteHandler.h"
#include "eventhandler/mousepress/ivm/obj/IVMSelectHandler.h"
#include "eventhandler/mouserelease/ivm/obj/IVMMouseReleaseHandler.h"
#include "mmm/MapWorkProject.h"
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

    // 初始化默认的鼠标释放处理器
    mrelease_handler = std::make_shared<IVMMouseReleaseHandler>();

    // 责任链式初始化鼠标拖动处理器
    // 先处理物件的移动
    mdrag_handler = std::make_shared<IVMDragMoveObjectHandler>();

    // 然后处理面条的编辑
    auto object_adjusthandler = std::make_shared<IVMDragAdjustObjectHandler>();
    mdrag_handler->set_next_handler(object_adjusthandler);
}

// 析构IVMObjectEditor
IVMObjectEditor::~IVMObjectEditor() = default;
// 撤销
void IVMObjectEditor::undo() { HitObjectEditor::undo(); }

// 重做
void IVMObjectEditor::redo() { HitObjectEditor::redo(); }

// 复制
void IVMObjectEditor::copy() {
    HitObjectEditor::copy();
    // 筛去非法复制部分
    check_editing_comp();
}
// 剪切
void IVMObjectEditor::cut() {
    HitObjectEditor::cut();
    // 筛去非法复制部分
    check_editing_comp();
}
// 粘贴
void IVMObjectEditor::paste() {
    // 根据第一个物件的时间戳到当前时间的偏移粘贴到editing_temp_objects或移动到editing_temp_objects
    if (is_cut) {
        // 剪切-依据clipboard直接修改editing_temp_objects
        if (!clipboard.empty()) {
            auto rtime = editor_ref->cstatus.current_time_stamp -
                         clipboard.begin()->get()->timestamp;
            for (const auto& o : editing_temp_objects) {
                auto note = std::dynamic_pointer_cast<Note>(o);
                if (note) {
                    switch (note->note_type) {
                        case NoteType::HOLD: {
                            auto hold = std::static_pointer_cast<Hold>(note);
                            hold->hold_end_reference->timestamp += rtime;
                            hold->timestamp += rtime;
                            break;
                        }
                        case NoteType::SLIDE: {
                            auto slide = std::static_pointer_cast<Slide>(note);
                            slide->slide_end_reference->timestamp += rtime;
                            slide->timestamp += rtime;
                            break;
                        }
                        case NoteType::NOTE: {
                            note->timestamp += rtime;
                            break;
                        }
                    }
                }
            }
        }
        end_edit();
        XINFO("已粘贴");
    } else {
        // 拷贝-读取剪切板
        if (!clipboard.empty()) {
            auto rtime = editor_ref->cstatus.current_time_stamp -
                         clipboard.begin()->get()->timestamp;
            for (const auto& src : clipboard) {
                auto note = std::dynamic_pointer_cast<Note>(src);
                if (!note) continue;
                // 不管子键了
                if (note->parent_reference) continue;

                switch (note->note_type) {
                    case NoteType::COMPLEX: {
                        auto comp = std::static_pointer_cast<ComplexNote>(note);
                        for (const auto& child_note : comp->child_notes) {
                            switch (child_note->note_type) {
                                case NoteType::HOLD: {
                                    auto hold = std::static_pointer_cast<Hold>(
                                        child_note);
                                    hold->hold_end_reference->timestamp +=
                                        rtime;
                                    hold->timestamp += rtime;
                                    break;
                                }
                                case NoteType::SLIDE: {
                                    auto slide =
                                        std::static_pointer_cast<Slide>(note);
                                    auto des =
                                        std::shared_ptr<Slide>(slide->clone());
                                    des->slide_end_reference->timestamp +=
                                        rtime;
                                    des->timestamp += rtime;
                                    editing_temp_objects.insert(des);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    case NoteType::HOLD: {
                        auto hold = std::static_pointer_cast<Hold>(note);
                        auto des = std::shared_ptr<Hold>(hold->clone());
                        des->hold_end_reference->timestamp += rtime;
                        des->timestamp += rtime;
                        editing_temp_objects.insert(des);
                        break;
                    }
                    case NoteType::SLIDE: {
                        auto slide = std::static_pointer_cast<Slide>(note);
                        auto des = std::shared_ptr<Slide>(slide->clone());
                        des->slide_end_reference->timestamp += rtime;
                        des->timestamp += rtime;
                        editing_temp_objects.insert(des);
                        break;
                    }
                    case NoteType::NOTE: {
                        auto des = std::shared_ptr<Note>(note->clone());
                        des->timestamp += rtime;
                        editing_temp_objects.insert(des);
                        break;
                    }
                }
            }
            update_current_comp();
            // 生成操作
            end_edit();
            XINFO("已拷贝");
        }
    }
}

// 结束编辑
void IVMObjectEditor::end_edit() {
    // 生成操作并执行到map
    // 生成编辑操作

    ObjEditOperation operation;

    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        editing_src_objects_temp;
    bool removed_parent{false};
    for (const auto& srcobj : editing_src_objects) {
        auto note = std::dynamic_pointer_cast<Note>(srcobj);
        std::shared_ptr<ComplexNote> removed_comp{nullptr};
        std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
            intersections;
        if (note) {
            switch (note->compinfo) {
                case ComplexInfo::HEAD:
                case ComplexInfo::BODY:
                case ComplexInfo::END: {
                    auto temp_comp = std::shared_ptr<ComplexNote>(
                        note->parent_reference, [](ComplexNote*) {});
                    if (temp_comp != removed_comp) {
                        // 有新的组合键内的物件
                        removed_comp = temp_comp;
                        // 取得交集
                        std::set_intersection(
                            editing_src_objects.begin(),
                            editing_src_objects.end(),
                            removed_comp->child_notes.begin(),
                            removed_comp->child_notes.end(),
                            std::inserter(intersections, intersections.end()),
                            editing_src_objects.key_comp());

                        // 判断交集是否完全属于组合键-(全选到了)
                        if (!mutil::full_selected_complex(intersections,
                                                          removed_comp)) {
                            XWARN("删除组合键部分");
                            std::multiset<std::shared_ptr<HitObject>,
                                          HitObjectComparator>
                                res;
                            // 不完全-需要拆分组合键
                            mutil::destruct_complex_note(res, removed_comp,
                                                         intersections);
                            editing_src_objects_temp.insert(
                                std::shared_ptr<HitObject>(
                                    removed_comp->clone()));
                            removed_parent = true;
                            for (const auto& obj : res) {
                                // 放入缓存
                                editing_temp_objects.insert(
                                    std::shared_ptr<HitObject>(obj->clone()));
                            }
                        } else {
                            // 全部包含-只需移除整个组合键
                            if (!removed_parent) {
                                XWARN("删除全部组合键");
                                removed_parent = true;
                                editing_src_objects_temp.insert(
                                    std::shared_ptr<HitObject>(
                                        temp_comp->clone()));
                            }
                        }
                    }
                    break;
                }
                default: {
                    editing_src_objects_temp.insert(
                        std::shared_ptr<HitObject>(srcobj->clone()));
                    break;
                }
            }
        } else {
            editing_src_objects_temp.insert(srcobj);
        }
    }

    // 处理编辑结果
    editing_src_objects = editing_src_objects_temp;
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

    // 添加操作标记
    editor_ref->operation_type_stack.emplace(std::make_pair(
        EditOperationType::EHITOBJECT, EditMethodPreference::IVM));
}

// 鼠标按下事件-传递
void IVMObjectEditor::mouse_pressed(QMouseEvent* e) {
    // 拍下悬浮位置快照
    if (editor_ref->ebuffer.hover_object_info) {
        hover_object_info_shortcut = std::shared_ptr<HoverObjectInfo>(
            editor_ref->ebuffer.hover_object_info->clone());
    } else {
        hover_object_info_shortcut = nullptr;
    }

    if (mpress_handler->handle(this, e, nearest_divisor_time(),
                               editor_ref->cstatus.mouse_pos_orbit)) {
        XINFO("鼠标按下事件已被处理");
    } else {
        XWARN("鼠标按下事件处理失败");
    }
}

void IVMObjectEditor::mouse_released(QMouseEvent* e) {
    if (mrelease_handler->handle(this, e, nearest_divisor_time(),
                                 editor_ref->cstatus.mouse_pos_orbit)) {
        XINFO("鼠标释放事件已被处理");
    } else {
        XWARN("鼠标释放事件处理失败");
    }
}

// 鼠标拖动事件-传递
void IVMObjectEditor::mouse_dragged(QMouseEvent* e) {
    if (!mdrag_handler->handle(this, e, nearest_divisor_time(),
                               editor_ref->cstatus.mouse_pos_orbit)) {
        XWARN("鼠标拖动事件处理失败");
    }
    // XINFO("鼠标拖动事件已被处理");
}

// 更新正编辑的当前组合键
void IVMObjectEditor::update_current_comp() {
    // 将当前编辑缓存的全部物件加入组合键子键中
    current_edit_complex->child_notes.clear();
    for (const auto& obj : editing_temp_objects) {
        auto temp_note = std::dynamic_pointer_cast<Note>(obj);
        if (!temp_note) continue;
        current_edit_complex->child_notes.insert(temp_note);
        temp_note->parent_reference = current_edit_complex.get();
    }
}

// 检查编辑中的组合键
void IVMObjectEditor::check_editing_comp() {
    std::shared_ptr<ComplexNote> temp_comp = nullptr;
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        illegal_edit_res;
    for (const auto& o : editing_src_objects) {
        std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
            temp_illegal_edit_res;
        auto comp = std::dynamic_pointer_cast<ComplexNote>(o);
        bool full_selected{true};
        if (comp) {
            // 包含组合键内容-检查是否完整包含
            for (const auto& child_note : comp->child_notes) {
                auto child_it = editing_src_objects.find(child_note);
                if (child_it == editing_src_objects.end()) {
                    // 此组合键不完全包含-去掉拷贝的此组合键的有关子键
                    full_selected = false;
                } else {
                    temp_illegal_edit_res.insert(child_note);
                }
            }
            if (full_selected) {
                // 全部选到，无需移除
            } else {
                // 未全选到-把组合键也移除
                temp_illegal_edit_res.insert(comp);
            }
        }
        // 收集非法物件
        for (const auto& temp_illegal_note : temp_illegal_edit_res) {
            illegal_edit_res.insert(temp_illegal_note);
        }
    }

    // 移除非法物件
    for (const auto& illegal_edit_note : illegal_edit_res) {
        auto it = editing_src_objects.find(illegal_edit_note);
        editing_src_objects.erase(it);
    }
}
