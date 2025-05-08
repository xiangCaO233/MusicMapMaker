#include "HitObjectEditor.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"

// 构造HitObjectEditor
HitObjectEditor::HitObjectEditor(MapEditor* meditor_ref)
    : editor_ref(meditor_ref) {}
// 析构HitObjectEditor
HitObjectEditor::~HitObjectEditor() {}

// 撤销
void HitObjectEditor::undo() {}

// 重做
void HitObjectEditor::redo() {}

// 鼠标最近的分拍线的时间
double HitObjectEditor::nearest_divisor_time() {
  // 查询当前鼠标位置最近的拍-在最近的分拍线放置
  auto beats = editor_ref->canvas_ref->working_map->query_beat_before_time(
      editor_ref->cstatus.mouse_pos_time);

  // 查到拍则获取最近的分拍线
  int32_t min_deviation_time = 2147483647;
  int32_t res_time;
  for (const auto& beat : beats) {
    for (int i = 0; i < beat->divisors; ++i) {
      auto div_time =
          beat->start_timestamp +
          i * (beat->end_timestamp - beat->start_timestamp) / beat->divisors;
      auto deviation_time =
          std::abs(editor_ref->cstatus.mouse_pos_time - div_time);
      if (deviation_time < min_deviation_time) {
        min_deviation_time = deviation_time;
        res_time = div_time;
      }
    }
  }
  return beats.empty() ? -1.0 : res_time;
}

// 鼠标按下事件-传递
void HitObjectEditor::mouse_pressed(QMouseEvent* e) {}

// 鼠标释放事件-传递
void HitObjectEditor::mouse_released(QMouseEvent* e) {}

// 鼠标拖动事件-传递
void HitObjectEditor::mouse_dragged(QMouseEvent* e) {}
