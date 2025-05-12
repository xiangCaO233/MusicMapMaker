#include "IVMEditComplexHandler.h"

#include "../../../../MapEditor.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "../../../HitObjectEditor.h"
#include "colorful-log.h"

// 构造IVMEditComplexHandler
IVMEditComplexHandler::IVMEditComplexHandler() {}

// 析构IVMEditComplexHandler
IVMEditComplexHandler::~IVMEditComplexHandler() = default;

// 处理事件
bool IVMEditComplexHandler::handle(HitObjectEditor* oeditor_context,
                                   QMouseEvent* e, double mouse_time,
                                   double mouse_orbit) {
    XINFO("处理组合键编辑事件");
    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
    auto note =
        std::dynamic_pointer_cast<Note>(ivmobjecteditor->current_edit_object);
    if (!note) return false;
    if (note->note_type == NoteType::HOLD) {
        // 1--缓存物件是面条
        auto hold = std::static_pointer_cast<Hold>(note);
        if (!hold) return false;

        // 已存在组合键且组合键尾为面条
        // 在面尾时间处添加一个滑键-更新当前编辑的物件为此新物件
        // 在组合键中添加滑键
        // 创建面条尾轨道到鼠标轨道的滑键
        // 上一个面条处于组合键中-必然不是位于组合头
        hold->compinfo = ComplexInfo::BODY;

        auto slide = std::make_shared<Slide>(
            mouse_time, hold->orbit,
            ivmobjecteditor->editor_ref->cstatus.mouse_pos_orbit - hold->orbit);
        // 修改当前正在编辑的物件为此滑键
        ivmobjecteditor->current_edit_object = slide;

        // 初始化滑尾
        slide->slide_end_reference = std::make_shared<SlideEnd>(slide);

        // 设置当前滑键为组合键尾
        slide->compinfo = ComplexInfo::END;

        // 记录物件快照
        ivmobjecteditor->info_shortcuts.insert(std::shared_ptr<HitObject>(
            ivmobjecteditor->current_edit_object->clone()));

        // 放入编辑缓存
        ivmobjecteditor->editing_temp_objects.insert(
            ivmobjecteditor->current_edit_object);
        ivmobjecteditor->update_current_comp();
        XINFO("为组合键新增滑键");
        return true;
    } else {
        // 2--缓存物件是滑键
        auto slide = std::static_pointer_cast<Slide>(note);
        if (!slide) return false;
        // 修改当前滑键为组合键body
        slide->compinfo = ComplexInfo::BODY;
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
        ivmobjecteditor->info_shortcuts.insert(std::shared_ptr<HitObject>(
            ivmobjecteditor->current_edit_object->clone()));
        // 放入编辑缓存
        ivmobjecteditor->editing_temp_objects.insert(
            ivmobjecteditor->current_edit_object);
        XINFO("为组合键新增面条");
        return true;
    }
}
