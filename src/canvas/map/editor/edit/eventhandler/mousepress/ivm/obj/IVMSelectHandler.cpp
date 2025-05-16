#include "IVMSelectHandler.h"

#include "../../../../../MapEditor.h"
#include "../../../../HitObjectEditor.h"
#include "colorful-log.h"
#include "mmm/hitobject/Note/Note.h"
#include "mmm/hitobject/Note/rm/ComplexNote.h"

// 构造IVMSelectHandler
IVMSelectHandler::IVMSelectHandler() {}
// 析构IVMSelectHandler
IVMSelectHandler::~IVMSelectHandler() = default;

// 选中物件
void IVMSelectHandler::select_note(IVMObjectEditor* ivmobjecteditor,
                                   HitObject* note) {
    // 克隆一份用于编辑缓存显示虚影
    auto cloned_obj = std::shared_ptr<HitObject>(note->clone());

    // 原来的物件放到src源物件表中-即将删除
    ivmobjecteditor->editing_src_objects.insert(
        std::shared_ptr<HitObject>(note, [](HitObject*) {}));

    // 克隆份放入编辑缓存
    ivmobjecteditor->editing_temp_objects.insert(cloned_obj);

    // 记录物件快照
    ivmobjecteditor->info_shortcuts.insert(
        std::shared_ptr<HitObject>(cloned_obj->clone()));
}

// 抄抄ivm编辑逻辑
// 处理事件
bool IVMSelectHandler::handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                              double mouse_time, double mouse_orbit) {
    XINFO("处理物件选择事件");
    // 检查是否有悬浮于某物件
    if (oeditor_context->editor_ref->ebuffer.hover_object_info) {
        auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
        // 看当前是否处于编辑面条状态-若为真则传递此次处理
        if (ivmobjecteditor->long_note_edit_mode)
            return next_handler->handle(oeditor_context, e, mouse_time,
                                        mouse_orbit);

        // 清空当前编辑的源物件表
        ivmobjecteditor->editing_src_objects.clear();

        // 更新当前操作的物件
        ivmobjecteditor->current_edit_object =
            ivmobjecteditor->editor_ref->ebuffer.hover_object_info->hoverobj;

        // 有悬浮于物件上
        // 检查是否有选中的大批物件
        if (!oeditor_context->editor_ref->ebuffer.selected_hitobjects.empty()) {
            // 选中已选中的全部物件
            for (const auto& selection :
                 ivmobjecteditor->editor_ref->ebuffer.selected_hitobjects) {
                if (!selection->is_note) continue;

                // 克隆一份用于编辑缓存显示虚影
                auto cloned_obj =
                    std::shared_ptr<HitObject>(selection->clone());

                // 原来的物件放到src源物件表中-即将删除
                ivmobjecteditor->editing_src_objects.insert(selection);

                // 记录物件快照
                ivmobjecteditor->info_shortcuts.insert(
                    std::shared_ptr<HitObject>(cloned_obj->clone()));
                ivmobjecteditor->editing_temp_objects.insert(cloned_obj);
            }
            // 清空选中
            ivmobjecteditor->editor_ref->ebuffer.selected_hitobjects.clear();
            XWARN("开始编辑已选中的物件");
        } else {
            // 判断是否选到组合键中的子键
            // 仅选中悬浮位置的物件
            // select_note(ivmobjecteditor,
            // ivmobjecteditor->current_edit_object);
            // XWARN("开始编辑鼠标悬浮物件");

            auto note = std::dynamic_pointer_cast<Note>(
                ivmobjecteditor->current_edit_object);
            if (note && note->compinfo != ComplexInfo::NONE) {
                // 如果选到组合键头的头部-把所有的都加入(整体)
                if (note->compinfo == ComplexInfo::HEAD &&
                    oeditor_context->editor_ref->ebuffer.hover_object_info
                            ->part == HoverPart::HEAD) {
                    // 选中组合键中所有的物件
                    select_note(ivmobjecteditor, note->parent_reference);
                    XWARN("开始编辑鼠标悬浮完整组合物件");
                } else {
                    // 仅选中悬浮位置的物件
                    select_note(ivmobjecteditor,
                                ivmobjecteditor->current_edit_object.get());
                    XWARN("开始编辑鼠标悬浮组合物件子键");
                }
            } else {
                // 仅选中悬浮位置的物件
                select_note(ivmobjecteditor,
                            ivmobjecteditor->current_edit_object.get());
                XWARN("开始编辑鼠标悬浮物件");
            }
        }

        // 左键仅选中-直接返回处理结果
        // 右键释放时物件直接删除-生成操作
        return true;
    } else {
        // 未悬浮于任何物件-清空选中
        if (!oeditor_context->editor_ref->ebuffer.selected_hitobjects.empty())
            oeditor_context->editor_ref->ebuffer.selected_hitobjects.clear();

        XINFO("未悬浮于任何物件-传递事件给放置物件处理器");

        // 传递事件-下一个应为放置物件的press处理器
        return next_handler->handle(oeditor_context, e, mouse_time,
                                    mouse_orbit);
    }
}
