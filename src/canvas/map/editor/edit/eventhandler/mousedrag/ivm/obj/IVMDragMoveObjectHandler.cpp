#include "IVMDragMoveObjectHandler.h"

#include <memory>

#include "../../../../../../MapWorkspaceCanvas.h"
#include "../../../../../MapEditor.h"
#include "../../../../IVMObjectEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "colorful-log.h"

// 构造IVMDragMoveObjectHandler
IVMDragMoveObjectHandler::IVMDragMoveObjectHandler() {}

// 析构IVMDragMoveObjectHandler
IVMDragMoveObjectHandler::~IVMDragMoveObjectHandler() = default;

// 处理事件
bool IVMDragMoveObjectHandler::handle(HitObjectEditor* oeditor_context,
                                      QMouseEvent* e, double mouse_time,
                                      double mouse_orbit) {
    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
    // 正处于面条编辑模式-放弃拖动事件的处理
    if (ivmobjecteditor->long_note_edit_mode) return false;

    if (!(std::abs(mouse_time - (-1.0)) < 1e-16)) {
        if (ivmobjecteditor->hover_object_info_shortcut) {
            // 根据编辑模式修改所有编辑缓存物件的
            // 计算相对变化
            auto note = std::dynamic_pointer_cast<Note>(
                ivmobjecteditor->current_edit_object);
            if (!note) return false;
            // 拖动单键
            // 相对移动的时间和轨道
            auto rtime = mouse_time - note->timestamp;
            auto rorbit = ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit -
                          note->orbit;

            switch (ivmobjecteditor->hover_object_info_shortcut->part) {
                case HoverPart::HEAD: {
                    // 拖动某个物件的头部-整体移动
                    // note
                    move_temp_objectsrelatively(ivmobjecteditor, rtime, rorbit);
                    break;
                }
                case HoverPart::HOLD_END: {
                    // 拖动某个面条的尾部
                    // hold
                    // 到下一处理器去调整持续时间
                    next_handler->handle(oeditor_context, e, mouse_time,
                                         mouse_orbit);
                    break;
                }
                case HoverPart::SLIDE_END: {
                    // 拖动某个滑键的尾部
                    // slide
                    // 到下一处理器去或滑动轨道数
                    next_handler->handle(oeditor_context, e, mouse_time,
                                         mouse_orbit);
                    break;
                }
                default: {
                    // ivm暂不支持
                    XWARN("ivm模式只能拖头或尾");
                    return false;
                }
            }
            return true;

        } else {
            // 拖着松开了
            return false;
        }
    } else {
        XWARN("找不到附近的分拍线");
        return false;
    }
}

double last_res_rtime;
int32_t last_res_rorbit;
std::shared_ptr<HitObject> last_check_obj{nullptr};

// 相对地移动缓存物件
void IVMDragMoveObjectHandler::move_temp_objectsrelatively(
    IVMObjectEditor* ivmobjecteditor, double rtime, int32_t rorbit) {
    // 当前编辑物件的相对位置和时间变化值
    // 将编辑缓存中的所有可转化为Note的物件应用此变化
    // 同时遍历缓存和快照-
    auto it = ivmobjecteditor->editing_temp_objects.begin();
    auto info_it = ivmobjecteditor->info_shortcuts.begin();

    while (it != ivmobjecteditor->editing_temp_objects.end() &&
           info_it != ivmobjecteditor->info_shortcuts.end()) {
        auto tempnote = std::dynamic_pointer_cast<Note>(*it);
        auto info = std::dynamic_pointer_cast<Note>(*info_it);
        if (tempnote) {
            auto srctime = tempnote->timestamp;
            auto srco = tempnote->orbit;
            // 组合键要调整所有的子键-且需要检查其不可偏的最大可移动位置
            auto comp = std::dynamic_pointer_cast<ComplexNote>(tempnote);
            auto comp_info = std::dynamic_pointer_cast<ComplexNote>(info);
            if (comp) {
                // 组合物件需要检查所有子键是否合法

                // 确认是一次新的检查
                if (last_check_obj != tempnote) {
                    last_res_rtime = rtime;
                    last_res_rorbit = rorbit;
                    last_check_obj = tempnote;
                }

                // 是否应用此次拖动变化
                bool use_this_ralt{true};

                // 检查每一个子键应用变化是否合法
                auto child_it = comp->child_notes.begin();
                auto child_info_it = comp_info->child_notes.begin();
                while (child_it != comp->child_notes.end() &&
                       child_info_it != comp_info->child_notes.end()) {
                    // 仅检查是否合法
                    if (!check_object(ivmobjecteditor, *child_it,
                                      *child_info_it, rtime, rorbit, false)) {
                        // 有不合法,取消此次拖动rtime和rorbit结果
                        use_this_ralt = false;
                        XWARN("检测到非法修改");
                        break;
                    }
                    ++child_it;
                    ++child_info_it;
                }

                // 应用此次变换
                if (use_this_ralt) {
                    last_res_rtime = rtime;
                    last_res_rorbit = rorbit;
                    child_it = comp->child_notes.begin();
                    child_info_it = comp_info->child_notes.begin();
                    while (child_it != comp->child_notes.end() &&
                           child_info_it != comp_info->child_notes.end()) {
                        check_object(ivmobjecteditor, *child_it, *child_info_it,
                                     rtime, rorbit, true);
                        ++child_it;
                        ++child_info_it;
                    }
                }
            } else {
                check_object(ivmobjecteditor, tempnote, info, rtime, rorbit,
                             true);
            }
            // XINFO(QString("移动物件t[%1]->t[%2]
            // o[%3]->o[%4]")
            //           .arg(srctime)
            //           .arg(tempnote->timestamp)
            //           .arg(srco)
            //           .arg(tempnote->orbit)
            //           .toStdString());
        }
        ++it;
        ++info_it;
    }
}
// 检查物件合法性
bool IVMDragMoveObjectHandler::check_object(IVMObjectEditor* ivmobjecteditor,
                                            std::shared_ptr<Note> tempnote,
                                            std::shared_ptr<Note> info,
                                            double rtime, int32_t rorbit,
                                            bool auto_adjust) {
    // 非组合物件只需检查本身位置是否合法
    tempnote->timestamp = info->timestamp + rtime;
    tempnote->orbit = info->orbit + rorbit;

    // 检查非法编辑结果
    // 非法时间位置
    if (tempnote->timestamp < 0) {
        if (!auto_adjust) {
            // 修复时间
            tempnote->timestamp = info->timestamp;
            return false;
        }
        tempnote->timestamp = 0;
        XWARN("检测到移动物件到非法时间位置");
        return false;
    }
    if (tempnote->timestamp >
        ivmobjecteditor->editor_ref->canvas_ref->working_map->map_length) {
        if (!auto_adjust) {
            // 修复时间
            tempnote->timestamp = info->timestamp;
            return false;
        }
        tempnote->timestamp =
            ivmobjecteditor->editor_ref->canvas_ref->working_map->map_length;
        XWARN("检测到移动物件到非法时间位置");
        return false;
    }

    // 非法轨道位置
    if (tempnote->orbit > ivmobjecteditor->editor_ref->ebuffer.max_orbit) {
        if (!auto_adjust) {
            tempnote->orbit = info->orbit;
            return false;
        }
        tempnote->orbit = ivmobjecteditor->editor_ref->ebuffer.max_orbit;
        XWARN("检测到移动物件到非法轨道位置");
        return false;
    }
    if (tempnote->orbit < 0) {
        if (!auto_adjust) {
            tempnote->orbit = info->orbit;
            return false;
        }
        tempnote->orbit = 0;
        XWARN("检测到移动物件到非法轨道位置");
        return false;
    }

    // 面条和滑键需要同时更新面尾和滑尾-组合键的时间戳也需要更新
    // TODO(xiang 2025-05-12):面尾和滑尾也许同步非法修改
    switch (tempnote->note_type) {
        case NoteType::HOLD: {
            auto hold = std::static_pointer_cast<Hold>(tempnote);
            hold->hold_end_reference->timestamp =
                tempnote->timestamp + hold->hold_time;
            if (hold->hold_end_reference->timestamp >
                ivmobjecteditor->editor_ref->canvas_ref->working_map
                    ->map_length) {
                if (!auto_adjust) {
                    // 恢复面头面尾
                    hold->timestamp = info->timestamp;
                    hold->hold_end_reference->timestamp =
                        tempnote->timestamp + hold->hold_time;
                    return false;
                }
                hold->hold_end_reference->timestamp =
                    ivmobjecteditor->editor_ref->canvas_ref->working_map
                        ->map_length;
                hold->hold_time =
                    hold->hold_end_reference->timestamp - hold->timestamp;
                XWARN("检测到移动面条尾到非法时间位置");
                return false;
            }

            break;
        }
        case NoteType::SLIDE: {
            auto slide = std::static_pointer_cast<Slide>(tempnote);
            slide->slide_end_reference->timestamp = slide->timestamp;
            slide->slide_end_reference->endorbit =
                slide->orbit + slide->slide_parameter;
            if (slide->slide_end_reference->endorbit >
                ivmobjecteditor->editor_ref->ebuffer.max_orbit) {
                if (!auto_adjust) {
                    // 恢复滑头滑尾
                    slide->orbit = info->orbit;
                    slide->slide_end_reference->endorbit =
                        slide->orbit + slide->slide_parameter;
                    return false;
                }
                slide->slide_end_reference->endorbit =
                    ivmobjecteditor->editor_ref->ebuffer.max_orbit;
                slide->slide_parameter =
                    slide->slide_end_reference->endorbit - slide->orbit;
                // TODO(xiang 2025-05-16):
                // 处理滑键的退化
                XWARN("检测到移动物件到非法轨道位置");
                return false;
            }
            if (slide->slide_end_reference->endorbit < 0) {
                if (!auto_adjust) return false;
                slide->slide_end_reference->endorbit = 0;
                slide->slide_parameter =
                    slide->slide_end_reference->endorbit - slide->orbit;
                // TODO(xiang 2025-05-16):
                // 处理滑键的退化
                XWARN("检测到移动物件到非法轨道位置");
                return false;
            }
            break;
        }
    }
    return true;
}
