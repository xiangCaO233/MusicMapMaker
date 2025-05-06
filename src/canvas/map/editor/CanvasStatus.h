#ifndef M_CANVAS_STATUS_H
#define M_CANVAS_STATUS_H

#include <QRectF>
#include <QSize>

#include "EditorEnumerations.h"
#include "mmm/map/MMap.h"

struct CanvasStatus {
  // 画布尺寸
  QSize canvas_size;

  // 正绑定的图类型
  MapType map_type;

  // 暂停播放
  bool canvas_pasued{true};

  // 播放速度
  double playspeed{1.0f};

  /*
   *编辑相关
   */
  // 是否悬停在某一物件上
  bool is_hover_note{false};
  // 是否悬停在某一timing上
  bool is_hover_timing{false};

  // 左键是否按下
  bool mouse_left_pressed{false};

  // 区域
  // 鼠标左键按下时的快照位置
  QPointF mouse_left_press_pos;

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
  double background_darken_ratio{0.75};

  // 时间偏移
  double static_time_offset{-15};

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
  bool is_magnet_to_divisor{false};
};

#endif  // M_CANVAS_STATUS_H
