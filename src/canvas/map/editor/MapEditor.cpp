#include "MapEditor.h"

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
  width_scale = (orbit_width - 4.0) / double(head_texture->width);

  // 不大于1--不放大纹理
  object_size_scale = std::min(width_scale, 1.0);
}
