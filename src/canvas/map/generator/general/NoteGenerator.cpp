#include "NoteGenerator.h"

#include <memory>

#include "../../../../mmm/hitobject/Note/Note.h"
#include "../../MapWorkspaceCanvas.h"
#include "../../editor/MapEditor.h"
#include "generator/ObjectGenerator.h"

// 构造NoteGenerator
NoteGenerator::NoteGenerator(std::shared_ptr<MapEditor>& editor)
    : ObjectGenerator(editor) {}

// 析构NoteGenerator
NoteGenerator::~NoteGenerator() = default;

// 生成物件渲染指令
void NoteGenerator::generate(Note& note) {
  auto note_ptr = std::shared_ptr<HitObject>(&note, [](HitObject*) {});
  auto head_note_size =
      QSizeF(editor_ref->head_texture->width * editor_ref->object_size_scale,
             editor_ref->head_texture->height * editor_ref->object_size_scale);
  double note_visual_time =
      editor_ref->current_visual_time_stamp +
      (note.timestamp - editor_ref->current_visual_time_stamp) *
          editor_ref->speed_zoom;

  // 物件距离判定线距离从下往上--反转
  // 当前物件头位置-中心
  auto note_center_pos_y =
      editor_ref->canvas_size.height() *
          (1.0 - editor_ref->judgeline_position) -
      ((editor_ref->canvas_pasued ? note.timestamp : note_visual_time) -
       editor_ref->current_visual_time_stamp) *
          editor_ref->timeline_zoom *
          (editor_ref->canvas_pasued ? 1.0 : editor_ref->speed_zoom);

  // 物件头中心位置
  auto note_center_pos_x = editor_ref->edit_area_start_pos_x +
                           editor_ref->orbit_width * note.orbit +
                           editor_ref->orbit_width / 2.0;

  // 物件头左上角位置
  double head_note_pos_x = note_center_pos_x - head_note_size.width() / 2.0;

  // 物件头的实际区域
  head_rect =
      QRectF(head_note_pos_x, note_center_pos_y - head_note_size.height() / 2.0,
             head_note_size.width(), head_note_size.height());

  // 节点
  const auto& complex_note_node_texture =
      editor_ref->canvas_ref->skin.get_object_texture(TexType::NODE,
                                                      ObjectStatus::COMMON);
  node_size =
      QSizeF(complex_note_node_texture->width * editor_ref->object_size_scale,
             complex_note_node_texture->height * editor_ref->object_size_scale);

  // 生成图形
  if (head_rect.contains(editor_ref->canvas_ref->mouse_pos)) {
    // 优先使用悬停时的纹理
    head_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::NOTE_HEAD, ObjectStatus::HOVER);
    editor_ref->hover_hitobject_info =
        std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(note_ptr,
                                                                      true);
    editor_ref->is_hover_note = true;
  } else {
    auto in_select_bound = editor_ref->strict_select
                               ? editor_ref->select_bound.contains(head_rect)
                               : editor_ref->select_bound.intersects(head_rect);
    auto it = editor_ref->selected_hitobjects.find(note_ptr);
    if (in_select_bound || it != editor_ref->selected_hitobjects.end()) {
      if (it == editor_ref->selected_hitobjects.end()) {
        // 未选中则选中此物件
        editor_ref->selected_hitobjects.emplace(note_ptr);
      }
      // 选中时的纹理
      head_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::NOTE_HEAD, ObjectStatus::SELECTED);
    } else {
      // 正常纹理
      head_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::NOTE_HEAD, ObjectStatus::COMMON);
    }
  }
}

// 完成物件头的生成-添加到渲染队列
void NoteGenerator::object_enqueue() {
  shape_queue.emplace(head_rect.x(), head_rect.y(), head_rect.width(),
                      head_rect.height(), head_texture);
}
