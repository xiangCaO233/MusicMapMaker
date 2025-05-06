#include "HoldGenerator.h"

#include <memory>

#include "../../../../mmm/MapWorkProject.h"
#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../MapWorkspaceCanvas.h"
#include "../../editor/MapEditor.h"
#include "mmm/hitobject/Note/Note.h"

HoldGenerator::HoldGenerator(std::shared_ptr<MapEditor> &editor)
    : NoteGenerator(editor) {}

HoldGenerator::~HoldGenerator() = default;

// body是否应显示悬浮
bool HoldGenerator::should_body_hover(
    const std::shared_ptr<Hold> &obj,
    const std::shared_ptr<HoverObjectInfo> &current_hoverinfo) const {
  // 当前没悬浮任何部分
  if (!current_hoverinfo) return true;
  // 悬浮到不同物件
  if (current_hoverinfo->hoverobj.get() != obj.get()) return true;
  switch (obj->compinfo) {
    case ComplexInfo::HEAD: {
      // 处于组合键头,若已hover于头或节点则不显示
      return !(current_hoverinfo->part == HoverPart::HEAD ||
               current_hoverinfo->part == HoverPart::COMPLEX_NODE);
    }
    case ComplexInfo::NONE: {
      // 不处于组合键中若已hover于头或尾则不显示
      return !(current_hoverinfo->part == HoverPart::HEAD ||
               current_hoverinfo->part == HoverPart::HOLD_END);
    }
    case ComplexInfo::BODY: {
      // 处于组合键内,若已hover于当前节点或上一节点(缓存节点back)则不显示
      if (temp_node_map.empty()) return true;
      auto hover_pre_node = temp_node_map.rbegin()->second.contains(
          editor_ref->canvas_ref->mouse_pos);
      return !(current_hoverinfo->part == HoverPart::COMPLEX_NODE ||
               hover_pre_node);
    }
    case ComplexInfo::END: {
      // 处于组合键尾,若已hover于尾或上一节点(缓存节点back)则不显示
      if (temp_node_map.empty()) return true;
      auto hover_pre_node = temp_node_map.rbegin()->second.contains(
          editor_ref->canvas_ref->mouse_pos);
      return !(current_hoverinfo->part == HoverPart::HOLD_END ||
               hover_pre_node);
    }
  }
  return false;
}

// 生成面条
void HoldGenerator::generate(Hold &hold) {
  NoteGenerator::generate(hold);
  auto hold_ptr = std::shared_ptr<Hold>(&hold, [](HitObject *) {});
  // 面条使用面尾标识
  objref = hold_ptr->hold_end_reference;
  // 添加long_note_body
  auto head_cp = head_rect.center();
  double long_note_end_visual_time =
      editor_ref->cstatus.current_visual_time_stamp +
      (hold.hold_end_reference->timestamp -
       editor_ref->cstatus.current_visual_time_stamp) *
          editor_ref->cstatus.speed_zoom;

  // 当前面条尾y轴位置
  auto long_note_end_pos_y =
      editor_ref->ebuffer.judgeline_position -
      ((editor_ref->cstatus.canvas_pasued ? hold.hold_end_reference->timestamp
                                          : long_note_end_visual_time) -
       editor_ref->cstatus.current_visual_time_stamp) *
          editor_ref->canvas_ref->working_map->project_reference->config
              .timeline_zoom;
  auto long_note_body_height = (long_note_end_pos_y - head_cp.y());

  // 当前面条身中心位置,y位置偏下一个note
  auto long_note_body_pos_x = head_cp.x();
  auto long_note_body_pos_y =
      head_cp.y() + (long_note_end_pos_y - head_cp.y()) / 2.0;

  auto &long_note_body_vertical_texture =
      editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_VERTICAL, ObjectStatus::COMMON);

  // 面身实际尺寸高度-0.5note
  auto long_note_body_size =
      QSizeF(long_note_body_vertical_texture->width *
                 editor_ref->ebuffer.object_size_scale *
                 editor_ref->canvas_ref->working_map->project_reference->config
                     .object_width_ratio,
             long_note_body_height);

  // 面身的实际区域--
  hold_vert_body_rect =
      QRectF(long_note_body_pos_x - long_note_body_size.width() / 2.0,
             long_note_body_pos_y - long_note_body_size.height() / 2.0,
             long_note_body_size.width(), long_note_body_size.height());

  auto &long_note_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
      TexType::HOLD_END, ObjectStatus::COMMON);

  // 添加long_note_end
  // 当前面条尾中心位置
  auto long_note_end_pos_x = head_cp.x();
  // 面尾实际尺寸
  auto long_note_end_size =
      QSizeF(long_note_end_texture->width *
                 editor_ref->ebuffer.object_size_scale * 1.1 *
                 editor_ref->canvas_ref->working_map->project_reference->config
                     .object_width_ratio,
             long_note_end_texture->height *
                 editor_ref->ebuffer.object_size_scale * 1.1 *
                 editor_ref->canvas_ref->working_map->project_reference->config
                     .object_height_ratio);

  // 先添加body图形
  // 是否有鼠标悬停
  bool is_hover_body =
      hold_vert_body_rect.contains(editor_ref->canvas_ref->mouse_pos);
  // 是否选中
  auto body_in_select_bound =
      editor_ref->csettings.strict_select
          ? editor_ref->ebuffer.select_bound.contains(hold_vert_body_rect)
          : editor_ref->ebuffer.select_bound.intersects(hold_vert_body_rect);
  auto body_selected_it =
      editor_ref->ebuffer.selected_hitobjects.find(hold_ptr);

  if (is_hover_body) {
    // 若已悬浮于长条头或尾或组合键节点,不切换悬浮部分和纹理使用
    auto &hoverinfo = editor_ref->ebuffer.hover_object_info;
    if (should_body_hover(hold_ptr, hoverinfo)) {
      hold_vert_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_VERTICAL, ObjectStatus::HOVER);
      hoverinfo = std::make_shared<HoverObjectInfo>(hold_ptr, hold.beatinfo,
                                                    HoverPart::HOLD_BODY);
      editor_ref->cstatus.is_hover_note = true;
    }
  } else {
    if (body_in_select_bound ||
        body_selected_it != editor_ref->ebuffer.selected_hitobjects.end()) {
      editor_ref->ebuffer.selected_hitobjects.emplace(hold_ptr);
      // 使用选中纹理
      hold_vert_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_VERTICAL, ObjectStatus::SELECTED);
      // 发送更新选中物件信号
      emit editor_ref->canvas_ref->select_object(
          hold.beatinfo, hold_ptr, editor_ref->ebuffer.current_abs_timing);
    } else {
      // 使用默认纹理
      hold_vert_body_texture = editor_ref->canvas_ref->skin.get_object_texture(
          TexType::HOLD_BODY_VERTICAL, ObjectStatus::COMMON);
    }
  }
  // 直接添加图形到队列
  shape_queue.emplace(hold_vert_body_rect.x(), hold_vert_body_rect.y(),
                      hold_vert_body_rect.width(), hold_vert_body_rect.height(),
                      hold_vert_body_texture, objref);

  // 节点的实际区域--也在面身缺的那note位置
  QRectF node_bound(long_note_end_pos_x - node_size.width() / 2.0,
                    long_note_end_pos_y - node_size.height() / 2.0,
                    node_size.width(), node_size.height());

  // 面尾的实际区域--在面身缺的那一note位置
  hold_end_rect =
      QRectF(long_note_end_pos_x - long_note_end_size.width() / 2.0,
             long_note_end_pos_y - long_note_end_size.height() / 2.0,
             long_note_end_size.width(), long_note_end_size.height());
  // 面尾是否有鼠标悬停
  bool is_hover_hold_end =
      hold_end_rect.contains(editor_ref->canvas_ref->mouse_pos);
  // 面尾是否选中
  auto hold_end_in_select_bound =
      editor_ref->csettings.strict_select
          ? editor_ref->ebuffer.select_bound.contains(hold_end_rect)
          : editor_ref->ebuffer.select_bound.intersects(hold_end_rect);
  switch (hold.compinfo) {
    case ComplexInfo::NONE: {
      // 单独的面条,头尾都画
      // 尾
      if (is_hover_hold_end) {
        // 使用hover纹理
        hold_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
            TexType::HOLD_END, ObjectStatus::HOVER);
        editor_ref->ebuffer.hover_object_info =
            std::make_shared<HoverObjectInfo>(hold_ptr, hold.beatinfo,
                                              HoverPart::HOLD_END);
        editor_ref->cstatus.is_hover_note = true;
      } else {
        if (hold_end_in_select_bound ||
            body_selected_it != editor_ref->ebuffer.selected_hitobjects.end()) {
          // 未选中则选中此面尾
          editor_ref->ebuffer.selected_hitobjects.emplace(
              hold.hold_end_reference);
          // 使用选中的纹理
          hold_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
              TexType::HOLD_END, ObjectStatus::SELECTED);
          // 发送更新选中物件信号
          emit editor_ref->canvas_ref->select_object(
              hold.beatinfo, hold_ptr, editor_ref->ebuffer.current_abs_timing);
        } else {
          // 使用普通纹理
          hold_end_texture = long_note_end_texture;
        }
      }
      // 直接添加面尾图形到队列
      shape_queue.emplace(hold_end_rect.x(), hold_end_rect.y(),
                          hold_end_rect.width(), hold_end_rect.height(),
                          hold_end_texture, objref);

      // 面头--直接将NoteGenerator中结果enqueue
      NoteGenerator::object_enqueue();
      break;
    }
      //  面条是组合键中的子键
    case ComplexInfo::HEAD: {
      // 面条位于组合键中头
      // 先把面头enqueue-并在尾处追加一个节点
      NoteGenerator::object_enqueue();
      temp_node_map.try_emplace(hold_ptr, node_bound);
      break;
    }
    case ComplexInfo::BODY: {
      // 仅在尾处追加一个节点
      temp_node_map.try_emplace(hold_ptr, node_bound);
      break;
    }
    case ComplexInfo::END: {
      // 补齐节点,画个尾

      // 面条是否完全过了当前时间
      dump_nodes_to_queue(hold.hold_end_reference->timestamp <
                          editor_ref->cstatus.current_time_stamp);

      // 再添加个面尾图形到队列
      if (is_hover_hold_end) {
        // 使用hover纹理
        hold_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
            TexType::HOLD_END, ObjectStatus::HOVER);
        editor_ref->ebuffer.hover_object_info =
            std::make_shared<HoverObjectInfo>(hold_ptr, hold.beatinfo,
                                              HoverPart::HOLD_END);
        editor_ref->cstatus.is_hover_note = true;
      } else {
        if (hold_end_in_select_bound ||
            body_selected_it != editor_ref->ebuffer.selected_hitobjects.end()) {
          // 未选中则选中此面尾
          editor_ref->ebuffer.selected_hitobjects.emplace(
              hold.hold_end_reference);
          // 使用选中的纹理
          hold_end_texture = editor_ref->canvas_ref->skin.get_object_texture(
              TexType::HOLD_END, ObjectStatus::SELECTED);
          // 发送更新选中物件信号
          emit editor_ref->canvas_ref->select_object(
              hold.beatinfo, objref, editor_ref->ebuffer.current_abs_timing);
        } else {
          // 使用普通纹理
          hold_end_texture = long_note_end_texture;
        }
      }
      shape_queue.emplace(hold_end_rect.x(), hold_end_rect.y(),
                          hold_end_rect.width(), hold_end_rect.height(),
                          hold_end_texture, objref);
      break;
    }
  }
}

// 生成面条
void HoldGenerator::generate_preview(Hold &hold) {}
