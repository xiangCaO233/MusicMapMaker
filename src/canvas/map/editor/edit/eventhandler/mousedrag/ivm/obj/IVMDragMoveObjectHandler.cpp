#include "IVMDragMoveObjectHandler.h"

#include "../../../../../../MapWorkspaceCanvas.h"
#include "../../../../../MapEditor.h"
#include "../../../../IVMObjectEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
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
            // 拖动单键
            // 相对移动的时间和轨道
            auto rtime = mouse_time - note->timestamp;
            auto rorbit = ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit -
                          note->orbit;

            switch (ivmobjecteditor->hover_object_info_shortcut->part) {
                case HoverPart::HEAD: {
                    // 拖动某个物件的头部-整体移动
                    // note
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
                            tempnote->timestamp = info->timestamp + rtime;
                            tempnote->orbit = info->orbit + rorbit;

                            // 检查非法编辑结果
                            if (tempnote->timestamp < 0) {
                                tempnote->timestamp = 0;
                            }
                            if (tempnote->orbit > ivmobjecteditor->editor_ref
                                                      ->ebuffer.max_orbit) {
                                tempnote->orbit = ivmobjecteditor->editor_ref
                                                      ->ebuffer.max_orbit;
                            }
                            if (tempnote->orbit < 0) {
                                tempnote->orbit = 0;
                            }

                            // 面条和滑键需要同时更新面尾和滑尾-组合键的时间戳也需要更新
                            // TODO(xiang 2025-05-12):面尾和滑尾也许同步非法修改
                            switch (tempnote->note_type) {
                                case NoteType::HOLD: {
                                    auto hold = std::static_pointer_cast<Hold>(
                                        tempnote);
                                    hold->hold_end_reference->timestamp =
                                        tempnote->timestamp + hold->hold_time;
                                    break;
                                }
                                case NoteType::SLIDE: {
                                    auto slide =
                                        std::static_pointer_cast<Slide>(
                                            tempnote);
                                    slide->slide_end_reference->timestamp =
                                        slide->timestamp;
                                    slide->slide_end_reference->endorbit =
                                        slide->orbit + slide->slide_parameter;
                                    break;
                                }
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
                    XWARN("只能拖头或尾");
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
