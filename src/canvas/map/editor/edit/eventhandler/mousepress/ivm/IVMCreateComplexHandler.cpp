#include "IVMCreateComplexHandler.h"

#include "../../../../MapEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "../../../HitObjectEditor.h"
#include "colorful-log.h"

// 构造IVMCreateComplexHandler
IVMCreateComplexHandler::IVMCreateComplexHandler() {}

// 析构IVMCreateComplexHandler
IVMCreateComplexHandler::~IVMCreateComplexHandler() = default;

// 处理事件
bool IVMCreateComplexHandler::handle(HitObjectEditor* oeditor_context,
                                     QMouseEvent* e, double mouse_time,
                                     double mouse_orbit) {
    XINFO("处理组合键生成事件");
    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
    if (!ivmobjecteditor->current_edit_complex) {
        XWARN("创建新组合键");
        auto note = std::dynamic_pointer_cast<Note>(
            ivmobjecteditor->current_edit_object);
        if (!note) return false;
        // 修改缓存中的面条物件的持续时间
        // 判断缓存物件是面条还是滑键
        if (note->note_type == NoteType::HOLD) {
            // 1--缓存物件是面条
            auto hold = std::static_pointer_cast<Hold>(note);
            if (!hold) return false;

            // 判断当前编辑物件的轨道与当前鼠标位置的轨道
            // 1.2当前编辑物件的轨道与当前鼠标位置的轨道必然不同-上一责任确认过
            // 判断当前编辑面条的面尾时间与鼠标时间是否相同
            if (hold->hold_end_reference->timestamp == mouse_time) {
                // 1.2.1当前编辑面条的面尾时间与鼠标时间相同
                // 此时必然已定义面条(上一责任确认过)-在面条头位置生成组合键
                ivmobjecteditor->current_edit_complex =
                    std::make_shared<ComplexNote>(hold->timestamp, hold->orbit);
                // 设置当前面条为组合键头
                hold->compinfo = ComplexInfo::HEAD;

                // 在组合键中添加滑键
                // 创建面条尾轨道到鼠标轨道的滑键
                auto slide = std::make_shared<Slide>(
                    mouse_time, hold->orbit,
                    ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit -
                        hold->orbit);
                // 修改当前正在编辑的物件为此滑键
                ivmobjecteditor->current_edit_object = slide;

                // 初始化滑尾
                slide->slide_end_reference = std::make_shared<SlideEnd>(slide);

                // 设置当前滑键为组合键尾
                slide->compinfo = ComplexInfo::END;

                // 记录物件快照
                ivmobjecteditor->info_shortcuts.insert(
                    std::shared_ptr<HitObject>(
                        ivmobjecteditor->current_edit_object->clone()));

                // 放入编辑缓存
                ivmobjecteditor->editing_temp_objects.insert(
                    ivmobjecteditor->current_edit_object);

                ivmobjecteditor->update_current_comp();

                XINFO("新建组合键");

                return true;
            } else {
                // 1.2.2当前编辑面条的面尾时间与鼠标时间不同-组合物件非法,跳过修改并提示
                XERROR("往哪滑呢😠 " +
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
        } else {
            // 2--缓存物件是滑键
            // 判断当前编辑滑键的时间与鼠标时间是否相同
            auto slide = std::static_pointer_cast<Slide>(note);
            if (!slide) return false;
            // 2.2当前编辑滑键的时间与鼠标时间必然不同(上一责任确认过)
            // 判断滑键尾轨道与鼠标位置轨道是否相同
            if (slide->slide_end_reference->endorbit ==
                ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit) {
                // 2.2.1滑键尾轨道与鼠标位置轨道相同
                // 生成组合键
                ivmobjecteditor->current_edit_complex =
                    std::make_shared<ComplexNote>(slide->timestamp,
                                                  slide->orbit);
                // 当前滑键为组合键头
                slide->compinfo = ComplexInfo::HEAD;
                // 生成组合键并新增长条到组合键
                // 新增长条到组合键
                auto hold = std::make_shared<Hold>(
                    slide->timestamp,
                    ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit,
                    mouse_time - slide->timestamp);
                // 面尾--
                hold->hold_end_reference = std::make_shared<HoldEnd>(hold);
                // 设置新增面条为组合键尾
                hold->compinfo = ComplexInfo::END;
                // 更新当前正在编辑的物件
                ivmobjecteditor->current_edit_object = hold;

                // 记录物件快照
                ivmobjecteditor->info_shortcuts.insert(
                    std::shared_ptr<HitObject>(
                        ivmobjecteditor->current_edit_object->clone()));
                // 放入编辑缓存
                ivmobjecteditor->editing_temp_objects.insert(
                    ivmobjecteditor->current_edit_object);

                ivmobjecteditor->update_current_comp();

                XINFO("新建组合键");
                return true;

            } else {
                // 2.2.2滑键尾轨道与鼠标位置轨道不同
                // 非法绘制
                XERROR("往哪加条呢😠 " +
                       QString("从滑尾位置-time[%1],orbit[%2]添加长条到-"
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
    } else {
        // 已有组合键-传递给编辑组合键处理器
        XWARN("继续编辑组合键");
        return next_handler->handle(oeditor_context, e, mouse_time,
                                    mouse_orbit);
    }
}
