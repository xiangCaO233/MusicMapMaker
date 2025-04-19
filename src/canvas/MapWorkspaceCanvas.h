#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include <qpaintdevice.h>
#include <qtmetamacros.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "AudioManager.h"
#include "GLCanvas.h"
#include "src/mmm/map/MMap.h"

// 鼠标正在操作的区域
enum class MouseOperationArea {
  // 编辑区
  EDIT,
  // 预览区
  PREVIEW,
  // 物件
  NOTE,
};

// "拍"结构体
struct Beat {
  double bpm;
  double start_timestamp;
  double end_timestamp;
  int32_t divisors{4};

  bool operator==(const Beat &other) const {
    return bpm == other.bpm && start_timestamp == other.start_timestamp &&
           end_timestamp == other.end_timestamp && divisors == other.divisors;
  }
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
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

  // 刷新定时器
  QTimer *refresh_timer;
  int32_t timer_update_time{8};

  // 暂停播放
  bool pasue{true};

  // 鼠标操作区
  MouseOperationArea operation_area;

  // 物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_objects;

  // 区域
  QRectF edit_area;
  QRectF preview_area;

  // 物件区域缓存--实时更新包含的物件,不包含得删
  std::unordered_map<std::shared_ptr<HitObject>, QRectF *> hitobject_bounds_map;

  // 当前正在使用的绝对timing--非变速timing
  std::shared_ptr<Timing> current_abs_timing;

  // 当前页面的拍
  std::vector<Beat> current_beats;

  // 滚动倍率
  double scroll_ratio{1.0};

  // 判定线位置:从下往上此倍率*总高度
  double judgeline_position{0.16};

  // 预览区宽度倍率:实际宽度为总宽度*preview_width_scale
  double preview_width_scale{0.22};

  // 预览区区域倍率:实际时间线缩放为timeline_zoom*preview_area_scale
  double preview_area_scale{5.0};

  // 绘制判定线
  void draw_judgeline();

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

  // 更新画布
  void update_canvas();

 public:
  // 构造MapWorkspaceCanvas
  explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

  // 析构MapWorkspaceCanvas
  ~MapWorkspaceCanvas() override;

  // 音频管理器
  std::shared_ptr<XAudioManager> audio_manager;

  // 分拍线颜色主题
  std::unordered_map<int32_t, std::vector<QColor>> divisors_color_theme;

  // 正在工作的图
  std::shared_ptr<MMap> working_map;

  // 当前时间戳
  double current_time_stamp{0};

  // 时间线缩放-- n * 1px/1ms
  double timeline_zoom{1.5};

  // 变速缩放
  double speed_zoom{1.0};

  // 切换到指定图
  void switch_map(std::shared_ptr<MMap> map);

  // 获取暂停状态
  inline bool &is_paused() { return pasue; }

  // 渲染实际图形
  // void push_shape() override;
 signals:
  void current_time_stamp_changed(double current_time_stamp);
};

#endif  // M_MAPWORKSPACE_H
