#include "SlideGenerator.h"

#include <memory>

#include "../../../../mmm/hitobject/Note/rm/Slide.h"
#include "../../MapWorkspaceCanvas.h"
#include "../../editor/MapEditor.h"
#include "generator/general/NoteGenerator.h"
#include "mmm/hitobject/Note/Note.h"

SlideGenerator::SlideGenerator(std::shared_ptr<MapEditor>& editor)
    : NoteGenerator(editor) {}

SlideGenerator::~SlideGenerator() {}

// 生成滑键
void SlideGenerator::generate(Slide& slide) {
  NoteGenerator::generate(slide);
  // 滑键
  auto slide_ptr = std::shared_ptr<Slide>(&slide, [](Slide*) {});
  objref = slide_ptr;
  auto endorbit = slide.orbit + slide.slide_parameter;

  // 横向身的终点位置
  auto horizon_body_end_pos_x = editor_ref->edit_area_start_pos_x +
                                editor_ref->orbit_width * endorbit +
                                editor_ref->orbit_width / 2.0;

  auto head_cp = head_rect.center();

  auto long_note_body_horizontal_texture =
      editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_HORIZONTAL, ObjectStatus::COMMON);

  // 横向身的尺寸
  // 横向身的高度
  auto horizon_body_height =
      long_note_body_horizontal_texture->height * editor_ref->object_size_scale;
  // 横向身的宽度
  auto horizon_body_width =
      std::abs(horizon_body_end_pos_x - head_cp.x()) + horizon_body_height;

  // 滑尾纹理
  std::shared_ptr<TextureInstace> slide_end_texture;
  std::shared_ptr<TextureInstace> slide_end_hovered_texture;
  std::shared_ptr<TextureInstace> slide_end_selected_texture;

  // 横向身的具体位置
  double horizon_body_pos_x = -horizon_body_height / 2.0;
  if (slide.slide_parameter > 0) {
    // 右滑,矩形就在头物件中心位置
    horizon_body_pos_x += head_cp.x();
    slide_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::SLIDE_END_RIGHT, ObjectStatus::COMMON);
    slide_end_hovered_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::SLIDE_END_RIGHT, ObjectStatus::HOVER);
    slide_end_selected_texture =
        editor_ref->canvas_ref->skin.get_object_texture(
            TexType::SLIDE_END_RIGHT, ObjectStatus::SELECTED);
  } else {
    // 左滑,矩形整体左移矩形宽度的位置
    horizon_body_pos_x = horizon_body_pos_x + head_cp.x() - horizon_body_width +
                         horizon_body_height;
    slide_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::SLIDE_END_LEFT, ObjectStatus::COMMON);
    slide_end_hovered_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::SLIDE_END_LEFT, ObjectStatus::HOVER);
    slide_end_selected_texture =
        editor_ref->canvas_ref->skin.get_object_texture(TexType::SLIDE_END_LEFT,
                                                        ObjectStatus::SELECTED);
  }

  // 横向身实际区域
  slide_hori_body_rect =
      QRectF(horizon_body_pos_x, head_cp.y() - horizon_body_height / 2.0,
             horizon_body_width, horizon_body_height);

  // 箭头位置--滑键结束轨道的位置
  slide_end_rect =
      QRectF(horizon_body_end_pos_x -
                 slide_end_texture->width * editor_ref->object_size_scale / 2.0,
             head_cp.y() - slide_end_texture->height *
                               editor_ref->object_size_scale / 2.0,
             slide_end_texture->width * editor_ref->object_size_scale,
             slide_end_texture->height * editor_ref->object_size_scale);

  // 先绘制横向身,然后头和箭头
  // 是否有鼠标悬停
  bool is_hover_body =
      slide_hori_body_rect.contains(editor_ref->canvas_ref->mouse_pos);
  // 是否选中横向身
  auto slide_blody_in_select_bound =
      editor_ref->strict_select
          ? editor_ref->select_bound.contains(slide_hori_body_rect)
          : editor_ref->select_bound.intersects(slide_hori_body_rect);
  auto body_selected_it = editor_ref->selected_hitobjects.find(slide_ptr);
  if (is_hover_body) {
    // 使用hover纹理
    slide_hori_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
        TexType::HOLD_BODY_HORIZONTAL, ObjectStatus::HOVER);
    editor_ref->hover_hitobject_info =
        std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(slide_ptr,
                                                                      false);
    editor_ref->is_hover_note = true;
  } else {
    if (slide_blody_in_select_bound ||
        body_selected_it != editor_ref->selected_hitobjects.end()) {
      // 选中此物件
      editor_ref->selected_hitobjects.emplace(slide_ptr);
      // 使用选中纹理
      slide_hori_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_HORIZONTAL, ObjectStatus::SELECTED);
      // 发送更新选中物件信号
      emit editor_ref->canvas_ref->select_object(
          objref, editor_ref->current_abs_timing);
    } else {
      // 使用常规纹理
      slide_hori_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_HORIZONTAL, ObjectStatus::COMMON);
    }
  }
  // 直接添加滑身到图形队列
  shape_queue.emplace(slide_hori_body_rect.x(), slide_hori_body_rect.y(),
                      slide_hori_body_rect.width(),
                      slide_hori_body_rect.height(), slide_hori_body_texture,
                      objref);

  // 节点的实际区域--也在面身缺的那note位置
  QRectF node_bound(horizon_body_end_pos_x - node_size.width() / 2.0,
                    head_cp.y() - node_size.height() / 2.0, node_size.width(),
                    node_size.height());

  // 滑尾是否有鼠标悬停
  bool is_hover_slide_end =
      slide_end_rect.contains(editor_ref->canvas_ref->mouse_pos);
  // 滑尾是否选中
  auto slide_end_in_select_bound =
      editor_ref->strict_select
          ? editor_ref->select_bound.contains(slide_end_rect)
          : editor_ref->select_bound.intersects(slide_end_rect);

  std::shared_ptr<TextureInstace> actual_use_end_texture;
  // 处理组合键中的情况
  switch (slide.compinfo) {
    case ComplexInfo::NONE: {
      // 单独的滑键,头尾都画
      // 头尾都画
      if (is_hover_slide_end) {
        // 使用hover纹理
        actual_use_end_texture = slide_end_hovered_texture;
        editor_ref->hover_hitobject_info =
            std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                slide.slide_end_reference, true);
        editor_ref->is_hover_note = true;
      } else {
        if (slide_end_in_select_bound ||
            body_selected_it != editor_ref->selected_hitobjects.end()) {
          // 选中此物件
          editor_ref->selected_hitobjects.emplace(slide_ptr);
          // 使用选中纹理
          actual_use_end_texture = slide_end_selected_texture;
          // 发送更新选中物件信号
          emit editor_ref->canvas_ref->select_object(
              objref, editor_ref->current_abs_timing);
        } else {
          // 使用常规纹理
          actual_use_end_texture = slide_end_texture;
        }
      }
      // 直接添加滑尾图形到队列
      shape_queue.emplace(slide_end_rect.x(), slide_end_rect.y(),
                          slide_end_rect.width(), slide_end_rect.height(),
                          actual_use_end_texture, objref);

      // 滑头--直接将NoteGenerator中结果enqueue
      NoteGenerator::object_enqueue();
      break;
    }
    case ComplexInfo::HEAD: {
      // 多画个头-在尾处追加一个节点
      NoteGenerator::object_enqueue();
      temp_node_map.try_emplace(slide_ptr, node_bound);
      break;
    }
    case ComplexInfo::BODY: {
      // 仅在尾处追加一个节点
      temp_node_map.try_emplace(slide_ptr, node_bound);
      break;
    }
    case ComplexInfo::END: {
      // 补齐节点,画个尾
      dump_nodes_to_queue();

      if (is_hover_slide_end) {
        // 使用hover纹理
        actual_use_end_texture = slide_end_hovered_texture;
        editor_ref->hover_hitobject_info =
            std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                slide.slide_end_reference, true);
        editor_ref->is_hover_note = true;
      } else {
        if (slide_end_in_select_bound ||
            body_selected_it != editor_ref->selected_hitobjects.end()) {
          // 选中此物件
          editor_ref->selected_hitobjects.emplace(slide_ptr);
          // 使用选中纹理
          actual_use_end_texture = slide_end_selected_texture;
          // 发送更新选中物件信号
          emit editor_ref->canvas_ref->select_object(
              objref, editor_ref->current_abs_timing);
        } else {
          // 使用常规纹理
          actual_use_end_texture = slide_end_texture;
        }
      }
      // 添加滑尾图形到队列
      shape_queue.emplace(slide_end_rect.x(), slide_end_rect.y(),
                          slide_end_rect.width(), slide_end_rect.height(),
                          actual_use_end_texture, objref);
      break;
    }
  }
}
