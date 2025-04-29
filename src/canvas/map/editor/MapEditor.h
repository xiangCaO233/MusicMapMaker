#ifndef M_MAPEDITOR_H
#define M_MAPEDITOR_H

#include <QPointF>
#include <QRectF>
#include <memory>
#include <unordered_set>
#include <vector>

#include "../../../mmm/Beat.h"
#include "../../../mmm/hitobject/HitObject.h"
#include "../../../mmm/timing/Timing.h"
#include "mmm/map/MMap.h"

class MapWorkspaceCanvas;
class TextureInstace;

enum class MouseEditMode {
  // 仅预览
  NONE,
  // 仅编辑,不新增物件,仅调整hover位置的物件
  EDIT,
  // 点击放置,拖动时调整时间戳
  PLACE_NOTE,
  // 拖动放置(组合键,面条,长条,滑键)
  PLACE_LONGNOTE,
};

// 鼠标正在操作的区域
enum class MouseOperationArea {
  // 编辑区
  EDIT,
  // 预览区
  PREVIEW,
  // 信息区
  INFO,
};

// 编辑器
class MapEditor {
 public:
  // 构造MapEditor
  explicit MapEditor(MapWorkspaceCanvas* canvas);

  // 析构MapEditor
  virtual ~MapEditor();

  // 画布引用
  MapWorkspaceCanvas* canvas_ref;

  // 画布尺寸
  QSize canvas_size;

  // 正绑定的图类型
  MapType map_type;

  // 暂停播放
  bool canvas_pasued{true};

  // 播放速度
  double playspeed{1.0f};

  // 显示时间线选项
  bool show_timeline{true};

  // 参考bpm
  std::unique_ptr<double> preference_bpm = nullptr;

  // 当前正在使用的绝对timing--非变速timing
  std::shared_ptr<Timing> current_abs_timing;

  // 附近的两个变速timing
  std::shared_ptr<Timing> pre_speed_timing;
  std::shared_ptr<Timing> next_speed_timing;

  // 当前页面的拍
  std::vector<std::shared_ptr<Beat>> current_beats;

  // 判定线位置:从下往上此倍率*总高度
  double judgeline_position{0.16};

  // 信息区宽度倍率:实际宽度为总宽度*preview_width_scale
  double infoarea_width_scale{0.16};

  // 预览区宽度倍率:实际宽度为总宽度*preview_width_scale
  double preview_width_scale{0.22};

  // 预览区时间倍率:实际时间范围为当前时间范围*preview_time_scale
  double preview_time_scale{10.0};

  // 上一次编辑区绘制的时间区域
  double current_time_area_start{0.0};
  double current_time_area_end{0.0};

  // 编辑区x起始位置
  double edit_area_start_pos_x;

  // 编辑区宽度
  double edit_area_width;

  // 轨道数
  int32_t max_orbit;

  // 物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_objects;

  // 预览物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_preview_objects;

  /*
   *编辑相关
   */
  // 是否悬停在某一物件上
  bool is_hover_note{false};

  // 鼠标悬停位置的物件(若不是面条尾或滑键尾,则bool为true时悬浮位置为头,false时为身)
  std::shared_ptr<std::pair<std::shared_ptr<HitObject>, bool>>
      hover_hitobject_info{nullptr};

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

  // 左键是否按下
  bool mouse_left_pressed{false};

  // 鼠标左键按下时的快照位置
  QPointF mouse_left_press_pos;

  // 选中框边框宽度(pix)
  double select_border_width{6};

  // 物件头的纹理
  std::shared_ptr<TextureInstace> head_texture;

  // 轨道宽度
  double orbit_width;

  // 依据轨道宽度自动适应物件纹理尺寸
  // 物件尺寸缩放--相对于纹理尺寸
  double width_scale;

  // 不大于1--不放大纹理
  double object_size_scale = std::min(width_scale, 1.0);

  // 选中框定位点
  std::shared_ptr<std::pair<QPointF, QPointF>> select_bound_locate_points;

  // 选中区域
  QRectF select_bound;

  // 是否严格选中
  bool strict_select{false};

  // 鼠标当前的编辑模式
  MouseEditMode edit_mode;

  // 区域
  // 鼠标当前的操作区
  MouseOperationArea operation_area;

  // 信息区
  QRectF info_area;

  // 编辑区
  QRectF edit_area;

  // 预览区
  QRectF preview_area;

  // 当前视觉时间戳
  double current_visual_time_stamp{0};

  // 当前时间戳
  double current_time_stamp{0};

  // 背景暗化
  double background_darken_ratio{0.55};

  // 时间偏移
  double static_time_offset{-15};

  // 用户调节的时间线缩放-- n * 1px/1ms
  double timeline_zoom{1.0};

  // TODO(xiang 2025-04-25):
  // 在两个变速timing间插值-下一个是基准timing时保持最后一个变速timing的速度作为时间线缩放
  // 变速缩放
  double speed_zoom{1.0};

  // 显示用的当前倍速
  double current_display_speed{1.0f};

  // 滚动倍率
  double scroll_ratio{1.0};

  // 滚动方向--1.0或-1.0
  double scroll_direction{1.0};

  // 滚动时吸附到小节线
  bool magnet_to_divisor{false};

  // 切换map
  void switch_map(std::shared_ptr<MMap>& map);

  // 画布更新尺寸
  void update_size(const QSize& current_canvas_size);
};

#endif  // M_MAPEDITOR_H
