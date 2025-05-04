#include "MapEditor.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../mmm/map/osu/OsuMap.h"
#include "../../mmm/map/rm/RMMap.h"
#include "../MapWorkspaceCanvas.h"

MapEditor::MapEditor(MapWorkspaceCanvas* canvas) : canvas_ref(canvas) {}

MapEditor::~MapEditor() {}

// 画布更新尺寸
void MapEditor::update_size(const QSize& current_canvas_size) {
  canvas_size = current_canvas_size;
  // 编辑区x起始位置
  edit_area_start_pos_x = current_canvas_size.width() * infoarea_width_scale;
  // 编辑区宽度
  edit_area_width = current_canvas_size.width() *
                    (1.0 - infoarea_width_scale - preview_width_scale);
  // 物件头的纹理
  head_texture = canvas_ref->skin.get_object_texture(TexType::NOTE_HEAD,
                                                     ObjectStatus::COMMON);

  if (!canvas_ref->working_map) return;

  // 更新轨道数
  switch (canvas_ref->working_map->maptype) {
    case MapType::OSUMAP: {
      // osu图
      auto omap = std::static_pointer_cast<OsuMap>(canvas_ref->working_map);
      max_orbit = omap->CircleSize;
      break;
    }
    case MapType::RMMAP: {
      // rm图
      auto rmmap = std::static_pointer_cast<RMMap>(canvas_ref->working_map);
      max_orbit = rmmap->max_orbits;
      break;
    }
    case MapType::MALODYMAP: {
      // ma图
      break;
    }
    default:
      break;
  }

  // 更新轨道宽度
  orbit_width = edit_area_width / max_orbit;

  // 依据轨道宽度自动适应物件纹理尺寸
  // 物件尺寸缩放--相对于纹理尺寸
  width_scale = (orbit_width * 1.2) / double(head_texture->width);

  // 不大于1--不放大纹理
  object_size_scale = std::min(width_scale, 1.0);
}

// 更新区域信息
void MapEditor::update_areas() {
  // 更新区域缓存
  // 信息区
  info_area.setX(0);
  info_area.setY(0);
  info_area.setWidth(canvas_size.width() * infoarea_width_scale);
  info_area.setHeight(canvas_size.height());

  // 编辑区
  edit_area.setX(0);
  edit_area.setY(0);
  edit_area.setWidth(canvas_size.width() * (1.0 - preview_width_scale));
  edit_area.setHeight(canvas_size.height());

  // 预览区
  preview_area.setX(canvas_size.width() * (1.0 - preview_width_scale));
  preview_area.setY(0);
  preview_area.setWidth(canvas_size.width() * preview_width_scale);
  edit_area.setHeight(canvas_size.height());
}

// 更新时间线缩放-滚动
void MapEditor::scroll_update_timelinezoom(int dy) {
  double res_timeline_zoom = timeline_zoom;
  if (dy > 0) {
    res_timeline_zoom += 0.05;
  } else {
    res_timeline_zoom -= 0.05;
  }
  if (res_timeline_zoom >= 0.2 && res_timeline_zoom <= 3.0) {
    timeline_zoom = res_timeline_zoom;
  }
}

// 吸附到附近分拍线
void MapEditor::magnet_to_divisor(int scrolldy) {
  // 获取当前时间附近的拍
  // 找到第一个拍起始时间大于或等于当前时间的拍迭代器
  auto current_beat_it = canvas_ref->working_map->beats.lower_bound(
      std::make_shared<Beat>(200, current_time_stamp, current_time_stamp));

  // 待吸附列表
  std::vector<std::shared_ptr<Beat>> magnet_beats;

  // 区分分支
  if (current_beat_it == canvas_ref->working_map->beats.end()) {
    // 没有比当前时间更靠后的拍了
    current_beat_it--;
    auto last_beat = *current_beat_it;
    // 添加最后一拍
    magnet_beats.emplace_back(last_beat);

    if (current_time_stamp == last_beat->start_timestamp) {
      // 在拍头--添加倒数第二拍的小节线和本拍小节线
      current_beat_it--;
      // auto second_tolast_beat = *current_beat_it;
      magnet_beats.emplace_back(*current_beat_it);
    } else if (current_visual_time_stamp > last_beat->start_timestamp &&
               current_visual_time_stamp < last_beat->end_timestamp) {
      // 在拍内--可只添加本拍小节线
    } else {
      // 在拍外--可只添加最后一拍的最后一个小节线
    }
  } else if (current_beat_it == canvas_ref->working_map->beats.begin()) {
    // 只有第一拍比当前时间更靠后了-比拍头更靠前或在拍头
    auto first_beat = *current_beat_it;
    magnet_beats.emplace_back(first_beat);
    // if (current_time_stamp < first_beat->start_timestamp) {
    //   // 在拍外--只添加第一拍的拍头
    //   magnet_beats.emplace_back(first_beat);
    // } else {
    //   // 在拍头--只添加本拍小节线
    //   magnet_beats.emplace_back(first_beat);
    // }
    //  在拍内--不会走此分支
  } else {
    // 附近都有拍--本拍前一拍后一拍的所有小节线全部加入吸附
    // auto current_beat = *current_beat_it;
    magnet_beats.emplace_back(*current_beat_it);
    --current_beat_it;
    // auto previous_beat = *current_beat_it;
    magnet_beats.emplace_back(*current_beat_it);
    ++current_beat_it;
    ++current_beat_it;
    if (current_beat_it != canvas_ref->working_map->beats.end()) {
      // auto next_beat = *current_beat_it;
      magnet_beats.emplace_back(*current_beat_it);
    }
  }
  // 待吸附时间集合
  std::set<double> divisor_times;
  // 初始化时间集合
  for (const auto& magnet_beat : magnet_beats) {
    for (int i = 0; i < magnet_beat->divisors; i++) {
      divisor_times.insert(
          magnet_beat->start_timestamp +
          i * (magnet_beat->end_timestamp - magnet_beat->start_timestamp) /
              magnet_beat->divisors);
    }
  }

  // 找到第一个时间大于或等于当前时间的小节线时间迭代器
  auto current_divisor_it = divisor_times.lower_bound(current_time_stamp);
  // 根据滑动方向选择吸附哪一个
  if (scrolldy * scroll_direction > 0) {
    // 向上吸附小节线
    if (std::abs(*current_divisor_it - current_time_stamp) < 10) {
      // 防止非法访问最后一个元素后的元素
      auto lasttime = divisor_times.end();
      lasttime--;
      if (current_divisor_it != lasttime) {
        // 找到自己了-吸下一个
        ++current_divisor_it;
        current_time_stamp = *current_divisor_it;
      } else {
        // 不动
      }
    } else {
      // 选择的大于自己的第一个
      current_time_stamp = *current_divisor_it;
    }
  } else {
    // 防止非法访问第一个元素前的元素
    if (current_divisor_it != divisor_times.begin()) {
      // 向下吸附小节线--不管是找到自己还是找到下一个,吸上一个
      --current_divisor_it;
      current_time_stamp = *current_divisor_it;
    }
  }
}

// 更新谱面位置
void MapEditor::update_timepos(int scrolldy, bool is_shift_down) {
  double temp_scroll_ration{1.0};
  if (is_shift_down & Qt::ShiftModifier) {
    // 在编辑区-按下shift滚动
    // 短暂增加滚动倍率
    temp_scroll_ration = 3.0;
  }

  if (is_magnet_to_divisor) {
    magnet_to_divisor(scrolldy);
  } else {
    auto scroll_unit = (scrolldy > 0 ? 1.0 : -1.0) * timeline_zoom *
                       canvas_size.height() / 10.0;
    current_time_stamp +=
        scroll_unit * temp_scroll_ration * scroll_ratio * scroll_direction;
  }
  if (current_time_stamp < 0) {
    current_time_stamp = 0;
    if (!canvas_pasued) {
      canvas_pasued = true;
      emit canvas_ref->pause_signal(canvas_pasued);
    }
  }
  if (current_time_stamp > canvas_ref->working_map->map_length) {
    current_time_stamp = canvas_ref->working_map->map_length;
    if (!canvas_pasued) {
      canvas_pasued = true;
      emit canvas_ref->pause_signal(canvas_pasued);
    }
  }
  current_visual_time_stamp = current_time_stamp + static_time_offset;
  canvas_ref->played_effects_objects.clear();

  // 更新项目中自己的位置
  canvas_ref->working_map->project_reference->map_canvasposes.at(
      canvas_ref->working_map) = current_time_stamp;
  emit canvas_ref->current_time_stamp_changed(current_time_stamp);

  canvas_ref->effect_thread->sync_music_time(current_time_stamp);
}

// 更新选中信息
void MapEditor::update_selections(bool is_ctrl_down) {
  if (select_bound_locate_points) {
    // 关闭选中框
    select_bound_locate_points = nullptr;
    select_bound.setWidth(0);
    select_bound.setHeight(0);
  }

  if (edit_mode == MouseEditMode::SELECT) {
    // 选中模式
    if (!is_ctrl_down) {
      // 未按住controll清空选中列表
      selected_hitobjects.clear();
    }
    // 按住controll左键多选
    if (hover_hitobject_info) {
      // 有悬浮在物件上
      selected_hitobjects.emplace(hover_hitobject_info->first);
    }

    // 发送更新选中物件信号
    if (hover_hitobject_info) {
      emit canvas_ref->select_object(hover_hitobject_info->second,
                                     hover_hitobject_info->first,
                                     current_abs_timing);
    } else {
      emit canvas_ref->select_object(nullptr, nullptr, nullptr);
    }
  }
}

// 更新选中区域
void MapEditor::update_selection_area(QPoint&& p, bool ctrl_down) {
  if (!select_bound_locate_points) {
    select_bound_locate_points =
        std::make_shared<std::pair<QPointF, QPointF>>(mouse_left_press_pos, p);
  } else {
    select_bound_locate_points->second = p;
  }

  // 更新选中区域
  select_bound.setX(std::min(select_bound_locate_points->first.x(),
                             select_bound_locate_points->second.x()));
  select_bound.setY(std::min(select_bound_locate_points->first.y(),
                             select_bound_locate_points->second.y()));
  select_bound.setWidth(std::abs(select_bound_locate_points->first.x() -
                                 select_bound_locate_points->second.x()));
  select_bound.setHeight(std::abs(select_bound_locate_points->first.y() -
                                  select_bound_locate_points->second.y()));

  // 更新选中的物件内容
  if (!ctrl_down) {
    // 没按ctrl,先清空当前选中的
    selected_hitobjects.clear();
  }
}
