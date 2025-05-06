#include "PreviewGenerator.h"

#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"
#include "general/HoldGenerator.h"
#include "general/NoteGenerator.h"
#include "general/SlideGenerator.h"

// 构造PreviewGenerator
PreviewGenerator::PreviewGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {
  // 注册物件生成器
  // 单物件
  objgenerators[NoteType::NOTE] = std::make_shared<NoteGenerator>(editor);
  // 面条物件
  objgenerators[NoteType::HOLD] = std::make_shared<HoldGenerator>(editor);
  // 滑键物件
  objgenerators[NoteType::SLIDE] = std::make_shared<SlideGenerator>(editor);
}

// 析构PreviewGenerator
PreviewGenerator::~PreviewGenerator() = default;

// 生成预览
void PreviewGenerator::generate() {
  if (!editor_ref->canvas_ref->working_map) return;
  // 确定区域
  auto area_center_time = (editor_ref->ebuffer.current_time_area_end -
                           editor_ref->ebuffer.current_time_area_start) /
                          2.0;
  auto parea_start_time =
      area_center_time -
      std::abs(area_center_time - editor_ref->ebuffer.current_time_area_start) *
          editor_ref->csettings.preview_time_scale;
  auto parea_end_time =
      area_center_time +
      std::abs(area_center_time - editor_ref->ebuffer.current_time_area_end) *
          editor_ref->csettings.preview_time_scale;

  // 查询此区域内所有物件
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> temp_objects;
  editor_ref->canvas_ref->working_map->query_object_in_range(
      temp_objects, int32_t(parea_start_time), int32_t(parea_end_time), true);

  // 防止重复绘制
  std::unordered_map<std::shared_ptr<HitObject>, bool> drawed_objects;
  // 渲染物件
  // 计算图形
  for (const auto &obj : temp_objects) {
    if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX) continue;
    auto note = std::static_pointer_cast<Note>(obj);
    if (!note) continue;
    if (drawed_objects.find(note) == drawed_objects.end())
      drawed_objects.insert({note, true});
    else
      continue;
    if (note) {
      // 生成物件
      const auto &generator = objgenerators[note->note_type];
      generator->generate_preview(obj);
      generator->preview_object_enqueue();
      // if (note->note_type == NoteType::NOTE) {
      //   // 除单键外的物件已经处理了头
      // }
    }
  }

  // 切换纹理绘制方式为填充
  editor_ref->canvas_ref->renderer_manager->texture_fillmode =
      TextureFillMode::FILL;
  // 切换纹理绘制补齐方式为重采样
  editor_ref->canvas_ref->renderer_manager->texture_complementmode =
      TextureComplementMode::REPEAT_TEXTURE;
  editor_ref->canvas_ref->renderer_manager->texture_effect =
      TextureEffect::NONE;

  // 按计算层级渲染图形
  while (!ObjectGenerator::preview_shape_queue.empty()) {
    auto &shape = ObjectGenerator::preview_shape_queue.front();
    editor_ref->canvas_ref->renderer_manager->addRect(
        QRectF(shape.x, shape.y, shape.w, shape.h), shape.tex,
        QColor(0, 0, 0, 255), 0, true);
    ObjectGenerator::preview_shape_queue.pop();
  }

  // 绘制一层滤镜
  QRectF preview_area_bg_bound(editor_ref->ebuffer.preview_area_start_pos_x,
                               0.0, editor_ref->ebuffer.preview_area_width,
                               editor_ref->cstatus.canvas_size.height());
  editor_ref->canvas_ref->renderer_manager->addRect(
      preview_area_bg_bound, nullptr, QColor(6, 6, 6, 75), 0, false);

  // 预览区域判定线
  // renderer_manager->addLine(
  //     QPointF(current_size.width() * (1 - editor->preview_width_scale),
  //             current_size.height() / 2.0),
  //     QPointF(current_size.width(), current_size.height() / 2.0), 6, nullptr,
  //     QColor(0, 255, 255, 235), false);
}
