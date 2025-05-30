#include "IVMMouseReleaseHandler.h"

#include "../../../../IVMObjectEditor.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"

// 构造IVMMouseReleaseHandler
IVMMouseReleaseHandler::IVMMouseReleaseHandler() {}
// 析构IVMMouseReleaseHandler
IVMMouseReleaseHandler::~IVMMouseReleaseHandler() = default;

// 处理事件
bool IVMMouseReleaseHandler::handle(HitObjectEditor* oeditor_context,
                                    QMouseEvent* e, double mouse_time,
                                    double mouse_orbit) {
    auto ivmobjecteditor = static_cast<IVMObjectEditor*>(oeditor_context);
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
            if (!ivmobjecteditor->is_cut &&
                !ivmobjecteditor->long_note_edit_mode) {
                ivmobjecteditor->end_edit();
                XINFO(QString("编辑结束生成物件操作").toStdString());
            } else {
                // 编辑面条模式并非结束
            }
            break;
        }
        case Qt::RightButton: {
            // 释放的是右键
            if (!ivmobjecteditor->is_cut &&
                !ivmobjecteditor->long_note_edit_mode) {
                // TODO(xiang 2025-05-12): 删除组合键内的物件时拆分组合键

                //
                //
                //
                //
                //
                //
                //
                // 删除选中的全部物件
                if (!ivmobjecteditor->editing_src_objects.empty()) {
                    ivmobjecteditor->editing_temp_objects.clear();
                    ivmobjecteditor->end_edit();
                    XINFO("删除物件");
                } else {
                    // 未选中任何物件-不执行删除
                    XWARN("未选中任何物件,不执行删除");
                }
            } else {
                // 若为面条编辑模式则面条编辑结束
                // 结束面条编辑
                // 只添加当前编辑的组合键
                if (ivmobjecteditor->current_edit_complex) {
                    ivmobjecteditor->editing_temp_objects.clear();
                    ivmobjecteditor->editing_temp_objects.insert(
                        ivmobjecteditor->current_edit_complex);
                }
                ivmobjecteditor->end_edit();
                ivmobjecteditor->long_note_edit_mode = false;
                XINFO("结束编辑面条");
            }
            break;
        }
    }
    return true;
}
