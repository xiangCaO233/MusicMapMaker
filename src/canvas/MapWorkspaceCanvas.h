#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "GLCanvas.h"
#include "src/mmm/map/MMap.h"

class MapWorkspaceCanvas : public GLCanvas {
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

  // 物件缓存
  std::vector<std::shared_ptr<HitObject>> buffer_objects;

  // 物件区域缓存
  std::unordered_map<QRectF *, std::shared_ptr<HitObject>> hitobject_bounds_map;

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

  // 当前页面的拍
  std::vector<Beat> current_beats;

  // 判定线位置:从下往上此倍率*总高度
  double judgeline_position{0.16};

  // 绘制判定线
  void draw_judgeline();

  // 绘制拍
  void draw_beats();

  // 绘制物件
  void draw_hitobject();

  // 绘制顶部栏
  void draw_top_bar();

  // 绘制预览
  void draw_preview_content();

  // 更新画布
  void update_canvas();

 public:
  // 构造MapWorkspaceCanvas
  explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

  // 析构MapWorkspaceCanvas
  ~MapWorkspaceCanvas() override;

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

  // 渲染实际图形
  void push_shape() override;
};

#endif  // M_MAPWORKSPACE_H
