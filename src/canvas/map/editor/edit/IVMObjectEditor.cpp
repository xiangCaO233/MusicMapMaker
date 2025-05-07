#include "IVMObjectEditor.h"

#include <cmath>
#include <cstdlib>

#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "editor/edit/HitObjectEditor.h"
#include "mmm/hitobject/Note/Note.h"

// 构造IVMObjectEditor
IVMObjectEditor::IVMObjectEditor(MapEditor* meditor_ref)
    : HitObjectEditor(meditor_ref) {}

// 析构IVMObjectEditor
IVMObjectEditor::~IVMObjectEditor() = default;

// 鼠标按下事件-传递
void IVMObjectEditor::mouse_pressed(QMouseEvent* e) {
  // 抄抄ivm编辑逻辑

  // 鼠标按钮
  auto button = e->button();

  // 按钮行为
  auto behavior = e->type();

  switch (button) {
    case Qt::LeftButton: {
      // 左键放置
      // 查询当前鼠标位置最近的拍-在最近的分拍线放置
      auto beat = editor_ref->canvas_ref->working_map->query_beat_before_time(
          editor_ref->cstatus.mouse_pos_time);
      if (beat) {
        // 查到拍了-获取最近的分拍线
        int32_t min_deviation_time = 2147483647;
        int32_t res_time;
        for (int i = 0; i < beat->divisors; ++i) {
          auto div_time = beat->start_timestamp +
                          i * (beat->end_timestamp - beat->start_timestamp) /
                              beat->divisors;
          auto deviation_time =
              std::abs(editor_ref->cstatus.mouse_pos_time - div_time);
          if (deviation_time < min_deviation_time) {
            min_deviation_time = deviation_time;
            res_time = div_time;
          }
        }
        // 根据编辑模式向此时间戳添加物件
        switch (editor_ref->edit_mode) {
          case MouseEditMode::PLACE_NOTE: {
            // 直接放置物件
            // 生成物件
            auto obj = std::make_shared<Note>(res_time, 0);
            // 生成编辑操作
            ObjEditOperation operation;
            operation.src_objects.clear();
            operation.des_objects.insert(obj);

            // 对map执行操作
            editor_ref->canvas_ref->working_map->execute_edit_operation(
                operation);

            // 入操作栈
            operation_stack.emplace(obj);
            break;
          }
          case MouseEditMode::PLACE_LONGNOTE: {
            // 放0长面条头-进入面条编辑状态-此时不可切换模式(直到右键面条编辑结束)
            long_note_edit_mode = true;
            break;
          }
        }
      }

      break;
    }
    case Qt::RightButton: {
      // 右键删除
      break;
    }
  }
}

// 鼠标拖动事件-传递
void IVMObjectEditor::mouse_dragged(QMouseEvent* e) {}
