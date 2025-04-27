#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include <qpaintdevice.h>
#include <qpoint.h>
#include <qtmetamacros.h>

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../mmm/Beat.h"
#include "../mmm/map/MMap.h"
#include "GLCanvas.h"

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

struct HitEffectFrame {
  std::shared_ptr<TextureInstace> effect_texture;
  QRectF effect_bound;
};

class MapWorkspaceCanvas : public GLCanvas {
  Q_OBJECT
 protected:
  // qt事件
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

  void initializeGL() override;

  /*
   *图形相关
   */
  // 打击特效纹理序列
  std::vector<std::shared_ptr<TextureInstace>> hiteffects;

  // 轨道特效帧队列
  std::unordered_map<int32_t, std::queue<HitEffectFrame>> effects;

  // 实际刷新时间间隔
  double actual_update_time{0};

  // 暂停播放
  bool canvas_pasued{true};

  // 播放速度
  double playspeed{1.0f};

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

  // 物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_objects;

  // 预览物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_preview_objects;

  /*
   *编辑相关
   */
  // 鼠标悬停位置的物件(若不是面条尾或滑键尾,则bool为true时悬浮位置为头,false时为身)
  std::shared_ptr<std::pair<std::shared_ptr<HitObject>, bool>>
      hover_hitobject_info{nullptr};

  // 选中的物件
  std::unordered_set<std::shared_ptr<HitObject>> selected_hitobjects;

  // 左键是否按下
  bool mouse_left_pressed{false};
  QPointF mouse_left_press_pos;

  // 选中框边框宽度(pix)
  double select_border_width{10};

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

  /*
   *成员函数
   */
  // 更新谱面时间位置
  void update_mapcanvas_timepos();

  // 绘制判定线
  void draw_judgeline();

  // 绘制选中框
  void draw_select_bound();

  // 绘制信息区
  void draw_infoarea();

  // 绘制拍
  void draw_beats();

  // 绘制物件
  void draw_hitobject();

  // 绘制顶部栏
  void draw_top_bar();

  // 绘制背景
  void draw_background();

  // 绘制预览
  void draw_preview_content();

 public:
  // 构造MapWorkspaceCanvas
  explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

  // 析构MapWorkspaceCanvas
  ~MapWorkspaceCanvas() override;

  // 分拍线颜色主题
  std::unordered_map<int32_t, std::vector<QColor>> divisors_color_theme;

  // 正在工作的图
  std::shared_ptr<MMap> working_map;

  // 正在工作的图类型
  MapType map_type;

  // 当前视觉时间戳
  double current_visual_time_stamp{0};

  // 当前时间戳
  double current_time_stamp{0};

  // 背景暗化
  double background_darken_ratio{0.55};

  // 时间偏移
  double static_time_offset{-20};

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

  // 切换到指定图
  void switch_map(std::shared_ptr<MMap> map);

  // 获取暂停状态
  inline bool &is_paused() { return canvas_pasued; }

  // 渲染实际图形
  void push_shape() override;

  // 更新fps显示
  virtual void updateFpsDisplay(int fps) override;

 public slots:
  // 时间控制器暂停按钮触发
  void on_timecontroller_pause_button_changed(bool paused);

  // 时间控制器播放速度变化
  void on_timecontroller_speed_changed(double speed);

 signals:
  // 时间戳更新信号
  void current_time_stamp_changed(double current_time_stamp);
  // 绝对bpm变化信号
  void current_absbpm_changed(double bpm);
  // 时间线速度变化信号
  void current_timeline_speed_changed(double timeline_speed);
  // 是否暂停播放信号
  void pause_signal(bool paused);
};

#endif  // M_MAPWORKSPACE_H
