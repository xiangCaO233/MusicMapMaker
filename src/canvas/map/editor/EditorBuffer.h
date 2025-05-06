#ifndef M_EDITORBUFFER_H
#define M_EDITORBUFFER_H

#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#include "../../../mmm/Beat.h"
#include "../../../mmm/hitobject/HitObject.h"
#include "../../../mmm/timing/Timing.h"
#include "../../texture/Texture.h"

enum class HoverPart {
  // 悬浮于物件头
  HEAD,
  // 悬浮于长条身
  HOLD_BODY,
  // 悬浮于长条尾
  HOLD_END,
  // 悬浮于滑键尾
  SLIDE_END,
  // 悬浮于组合键节点
  COMPLEX_NODE,
};

struct HoverInfo {
  std::shared_ptr<HitObject> hoverobj;
  std::shared_ptr<Beat> hoverbeat;
  HoverPart part;
};

struct EditorBuffer {
  // 上一次编辑区绘制的时间区域
  double current_time_area_start{0.0};
  double current_time_area_end{0.0};

  // 当前正在使用的绝对timing--非变速timing
  std::shared_ptr<Timing> current_abs_timing;

  // 附近的两个变速timing
  std::shared_ptr<Timing> pre_speed_timing;
  std::shared_ptr<Timing> next_speed_timing;

  // 当前页面的拍
  std::vector<std::shared_ptr<Beat>> current_beats;

  // 编辑区x起始位置
  double edit_area_start_pos_x;

  // 编辑区宽度
  double edit_area_width;

  // 判定线位置
  double judgeline_position;

  // 判定线视觉位置
  double judgeline_visual_position;

  // 轨道数
  int32_t max_orbit;

  // 物件缓存
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> buffer_objects;

  // 预览物件缓存
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
      buffer_preview_objects;

  // 鼠标悬停位置的物件信息
  std::shared_ptr<HoverInfo> hover_info{nullptr};

  // 鼠标悬停位置的timings信息(可能是两个同时间timing)
  std::vector<std::shared_ptr<Timing>>* hover_timings{nullptr};

  // 选中的物件
  // 哈希函数和比较函数
  struct RawPtrHash {
    size_t operator()(const std::shared_ptr<HitObject>& ptr) const {
      return std::hash<HitObject*>()(ptr.get());
    }
  };

  struct RawPtrEqual {
    bool operator()(const std::shared_ptr<HitObject>& lhs,
                    const std::shared_ptr<HitObject>& rhs) const {
      return lhs.get() == rhs.get();
    }
  };
  std::unordered_set<std::shared_ptr<HitObject>, RawPtrHash, RawPtrEqual>
      selected_hitobjects;

  // 物件头的纹理
  std::shared_ptr<TextureInstace> head_texture;

  // 轨道宽度
  double orbit_width;

  // 依据轨道宽度自动适应物件纹理尺寸
  // 物件尺寸缩放--相对于纹理尺寸
  double width_scale;

  // 不大于1--不放大纹理
  double object_size_scale;

  // 选中框定位点
  std::shared_ptr<std::pair<QPointF, QPointF>> select_bound_locate_points;

  // 选中区域
  QRectF select_bound;
};

#endif  // M_EDITORBUFFER_H
