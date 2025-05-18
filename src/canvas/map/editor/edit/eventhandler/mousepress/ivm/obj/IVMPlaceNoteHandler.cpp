#include "IVMPlaceNoteHandler.h"

#include <qstring.h>

#include "../../../../../MapEditor.h"
#include "../../../../HitObjectEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "colorful-log.h"
#include "editor/info/HoverInfo.h"

// 构造IVMPlaceNoteHandler
IVMPlaceNoteHandler::IVMPlaceNoteHandler() {}
// 析构IVMPlaceNoteHandler
IVMPlaceNoteHandler::~IVMPlaceNoteHandler() = default;

// 处理事件
bool IVMPlaceNoteHandler::handle(HitObjectEditor* oeditor_context,
                                 QMouseEvent* e, double mouse_time,
                                 double mouse_orbit) {
    if (e->button() != Qt::LeftButton) return false;

    XINFO("处理物件放置事件");
    // 鼠标附近无可用拍线-处理失败
    if (std::abs(mouse_time - (-1.0)) <= 1e-7) {
        XWARN("找不到分拍线");
        return false;
    }

    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
    // 根据编辑模式-放置缓存物件或是传递事件
    switch (oeditor_context->editor_ref->edit_mode) {
        case MouseEditMode::PLACE_NOTE: {
            // 直接在鼠标位置创建单物件
            // 直接放置物件
            // 在对应轨道生成物件
            ivmobjecteditor->current_edit_object = std::make_shared<Note>(
                mouse_time,
                ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit);

            // 生成一个note信息快照记录鼠标按下时的位置信息
            // 记录物件快照
            auto shortcut = *ivmobjecteditor->info_shortcuts.insert(
                std::shared_ptr<HitObject>(
                    ivmobjecteditor->current_edit_object->clone()));

            // 放入编辑缓存
            ivmobjecteditor->editing_src_objects.clear();
            ivmobjecteditor->editing_temp_objects.insert(
                std::shared_ptr<HitObject>(
                    ivmobjecteditor->current_edit_object->clone()));

            // 放置temp物件同时生成缓存信息
            ivmobjecteditor->hover_object_info_shortcut =
                std::make_shared<HoverObjectInfo>();
            ivmobjecteditor->hover_object_info_shortcut->hoverobj = shortcut;
            ivmobjecteditor->hover_object_info_shortcut->part = HoverPart::HEAD;

            XINFO(QString("新增物件->[%1]")
                      .arg(ivmobjecteditor->current_edit_object->timestamp)
                      .toStdString());
            return true;
        }
        case MouseEditMode::PLACE_LONGNOTE: {
            // 放置面条并模式切换或传递事件给编辑组合键或创建组合键
            if (ivmobjecteditor->long_note_edit_mode) {
                // 处于面条编辑模式
                XWARN("正在编辑面条");

                // 上一次编辑的物件
                auto note = std::dynamic_pointer_cast<Note>(
                    ivmobjecteditor->current_edit_object);
                if (!note) return false;

                // 判断缓存物件是面条还是滑键
                if (note->note_type == NoteType::HOLD) {
                    // 1--缓存物件是面条
                    auto hold = std::static_pointer_cast<Hold>(note);
                    if (!hold) return false;

                    // 判断当前编辑物件的轨道与当前鼠标位置的轨道
                    if (note->orbit ==
                        ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit) {
                        // 1.1当前编辑物件的轨道与当前鼠标位置的轨道相同
                        // 判断当前编辑物件头的时间到鼠标时间>=0
                        if (mouse_time - note->timestamp >= 0) {
                            // 1.1.1快照时间到鼠标时间>=0时修改缓存物件面条中的持续时间
                            hold->hold_time = mouse_time - note->timestamp;
                            // 修改面尾位置
                            hold->hold_end_reference->timestamp =
                                hold->timestamp + hold->hold_time;
                            XWARN("编辑面条持续时间");
                            return true;
                        } else {
                            // 1.1.2快照时间到鼠标时间<0面条持续时间非法,跳过修改并提示
                            XERROR(QString("面条持续时间非法[%1]ms")
                                       .arg(mouse_time - note->timestamp)
                                       .toStdString());
                            return false;
                        }
                    } else {
                        // 1.2当前编辑物件的轨道与当前鼠标位置的轨道不同
                        // 判断当前编辑面条的面尾时间与鼠标时间是否相同
                        if (hold->hold_end_reference->timestamp == mouse_time) {
                            // 1.2.1当前编辑面条的面尾时间与鼠标时间相同
                            // 判断是否生成组合键
                            if (!ivmobjecteditor->current_edit_complex) {
                                // 未生成组合键-
                                if (hold->hold_time == 0) {
                                    // 处于未定义面条状态
                                    // 直接去掉面条改为滑键
                                    // 创建滑键
                                    auto slide = std::make_shared<Slide>(
                                        mouse_time, hold->orbit,
                                        ivmobjecteditor->editor_ref->cstatus
                                                .mouse_pos_orbit -
                                            hold->orbit);
                                    // 修改当前正在编辑的物件为此滑键
                                    ivmobjecteditor->current_edit_object =
                                        slide;

                                    // 初始化滑尾
                                    slide->slide_end_reference =
                                        std::make_shared<SlideEnd>(slide);

                                    // 清理编辑缓存
                                    ivmobjecteditor->editing_temp_objects
                                        .clear();

                                    // 清理快照
                                    ivmobjecteditor->info_shortcuts.clear();

                                    // 记录物件克隆快照
                                    ivmobjecteditor->info_shortcuts.insert(
                                        std::shared_ptr<HitObject>(
                                            ivmobjecteditor->current_edit_object
                                                ->clone()));

                                    // 放入编辑缓存
                                    ivmobjecteditor->editing_temp_objects
                                        .insert(ivmobjecteditor
                                                    ->current_edit_object);

                                    XINFO("修改正在编辑的物件为-滑键");
                                    return true;
                                } else {
                                    // 已定义面条-传递事件给组合键创建处理器
                                    return next_handler->handle(oeditor_context,
                                                                e, mouse_time,
                                                                mouse_orbit);
                                }
                            } else {
                                // 已存在组合键且组合键尾为面条
                                // 传递事件给组合键创建处理器
                                return next_handler->handle(oeditor_context, e,
                                                            mouse_time,
                                                            mouse_orbit);
                            }
                        } else {
                            // 1.2.2当前编辑面条的面尾时间与鼠标时间不同-组合物件非法,跳过修改并提示
                            XERROR(
                                "往哪滑呢😠 " +
                                QString("从面条尾-time[%1],orbit[%2]添加滑键到-"
                                        "time[%3],orbit[%4],slideorbit[%5]非法")
                                    .arg(hold->hold_end_reference->timestamp)
                                    .arg(hold->orbit)
                                    .arg(mouse_time)
                                    .arg(ivmobjecteditor->editor_ref->cstatus
                                             .mouse_pos_orbit)
                                    .arg(ivmobjecteditor->editor_ref->cstatus
                                             .mouse_pos_orbit -
                                         hold->orbit)
                                    .toStdString());
                            return false;
                        }
                    }
                } else {
                    // 2--缓存物件是滑键
                    // 判断当前编辑滑键的时间与鼠标时间是否相同
                    auto slide = std::static_pointer_cast<Slide>(note);
                    if (!slide) return false;
                    if (slide->timestamp == mouse_time) {
                        // 2.1当前编辑滑键的时间与鼠标时间相同
                        // 修改当前滑键的滑动参数
                        auto silde_param = ivmobjecteditor->editor_ref->cstatus
                                               .mouse_pos_orbit -
                                           slide->orbit;
                        if (silde_param == 0) {
                            XWARN("不允许原地滑😠");
                            return false;
                        } else {
                            // 更新滑键滑动轨道数
                            slide->slide_parameter = silde_param;
                            slide->slide_end_reference->endorbit =
                                slide->orbit + slide->slide_parameter;
                            return true;
                        }
                    } else {
                        // 2.2当前编辑滑键的时间与鼠标时间不同
                        // 判断滑键尾轨道与鼠标位置轨道是否相同
                        if (slide->slide_end_reference->endorbit ==
                            ivmobjecteditor->editor_ref->cstatus
                                .mouse_pos_orbit) {
                            // 2.2.1滑键尾轨道与鼠标位置轨道相同
                            // 时间不同-轨道相同-向上添加组合键
                            // 传递事件给生成组合键处理器
                            return next_handler->handle(
                                oeditor_context, e, mouse_time, mouse_orbit);
                        } else {
                            // 2.2.2滑键尾轨道与鼠标位置轨道不同
                            // 非法绘制
                            XERROR(
                                "往哪加条呢😠 " +
                                QString(
                                    "从滑尾位置-time[%1],orbit[%2]添加长条到-"
                                    "time[%3],orbit[%4],holdtime[%5]非法")
                                    .arg(slide->timestamp)
                                    .arg(slide->slide_end_reference->endorbit)
                                    .arg(mouse_time)
                                    .arg(ivmobjecteditor->editor_ref->cstatus
                                             .mouse_pos_orbit)
                                    .arg(mouse_time - slide->timestamp)
                                    .toStdString());
                            return false;
                        }
                    }
                }
            } else {
                // 不处于面条编辑模式-放置个面条并进入此模式
                // 生成虚影0长面条
                auto hold = std::make_shared<Hold>(
                    mouse_time,
                    ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit, 0);
                // 面尾--
                hold->hold_end_reference = std::make_shared<HoldEnd>(hold);

                ivmobjecteditor->current_edit_object = hold;

                // 生成一个note信息快照记录鼠标按下时的位置信息
                // 记录物件快照
                ivmobjecteditor->info_shortcuts.insert(
                    std::shared_ptr<HitObject>(
                        ivmobjecteditor->current_edit_object->clone()));
                ivmobjecteditor->editing_src_objects.clear();

                // 放入编辑缓存
                ivmobjecteditor->editing_temp_objects.insert(
                    ivmobjecteditor->current_edit_object);

                ivmobjecteditor->long_note_edit_mode = true;

                XINFO("开始编辑面条");
                return true;
            }
        }
        default:
            return false;
    }
}
