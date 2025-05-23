#ifndef M_IVMOBJECTEDITOR_H
#define M_IVMOBJECTEDITOR_H

#include <memory>

#include "HitObjectEditor.h"

class IVMObjectEditor : public HitObjectEditor {
   public:
    // 构造IVMObjectEditor
    IVMObjectEditor(MapEditor* meditor_ref);
    // 析构IVMObjectEditor
    ~IVMObjectEditor() override;

    // 面条编辑模式
    bool long_note_edit_mode{false};

    // 正在编辑的物件
    std::shared_ptr<HitObject> current_edit_object{nullptr};

    // 正在编辑的组合物件
    std::shared_ptr<ComplexNote> current_edit_complex{nullptr};

    // 正在编辑的物件信息快照
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        info_shortcuts;

    // 更新正编辑的当前组合键
    void update_current_comp();

    // 检查编辑中的组合键
    void check_editing_comp(std::multiset<std::shared_ptr<HitObject>,
                                          HitObjectComparator>& checking_set);

    // 结束编辑-生成可撤回操作入栈
    void end_edit();

    // 鼠标按下事件-传递
    void mouse_pressed(QMouseEvent* e) override;
    // 鼠标释放事件-传递
    void mouse_released(QMouseEvent* e) override;
    // 鼠标拖动事件-传递
    void mouse_dragged(QMouseEvent* e) override;

    // 撤销
    void undo() override;

    // 重做
    void redo() override;

    // 复制
    void copy() override;
    // 剪切
    void cut() override;
    // 粘贴
    void paste() override;

    // 粘贴剪切板内容
    void paste_clipboard();
};

#endif  // M_IVMOBJECTEDITOR_H
