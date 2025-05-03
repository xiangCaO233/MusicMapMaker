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

#include "../../mmm/Beat.h"
#include "../../mmm/map/MMap.h"
#include "../GLCanvas.h"
#include "MapWorkspaceSkin.h"
#include "editor/MapEditor.h"
#include "generator/AreaInfoGenerator.h"
#include "generator/BeatGenerator.h"
#include "generator/ObjectGenerator.h"
#include "generator/TimeInfoGenerator.h"
#include "threads/EffectThread.h"

enum class EffectType {
  NORMAL,
  SLIDEARROW,
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
  // 物件生成器
  std::unordered_map<NoteType, std::shared_ptr<ObjectGenerator>> objgenerators;

  // 节拍生成器
  std::shared_ptr<BeatGenerator> beatgenerator;

  // 时间区域信息生成器
  std::shared_ptr<AreaInfoGenerator> areagenerator;

  // 时间信息生成器
  std::shared_ptr<TimeInfoGenerator> timegenerator;

  // x位置-特效帧队列
  std::unordered_map<
      double, std::queue<std::pair<QRectF, std::shared_ptr<TextureInstace>>>>
      effect_frame_queue_map;

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

  // 实际的帧更新时间-qt
  double actual_update_time{0};

  // 编辑器
  std::shared_ptr<MapEditor> editor;

  // 效果线程
  std::unique_ptr<EffectThread> effect_thread;

  // 正在工作的图
  std::shared_ptr<MMap> working_map;

  // 皮肤
  MapWorkspaceSkin skin;

  // 播放过特效的物件
  std::unordered_set<std::shared_ptr<HitObject>> played_effects_objects;

  // 播放特效
  void play_effect(double xpos, double ypos, int32_t frame_count,
                   EffectType etype);

  // 切换到指定图
  void switch_map(std::shared_ptr<MMap> map);

  // 获取暂停状态
  inline bool &is_paused() { return editor->canvas_pasued; }

  // 渲染实际图形
  void push_shape() override;

  // 更新fps显示
  virtual void updateFpsDisplay(int fps) override;

 public slots:
  // 时间控制器暂停按钮触发
  void on_timecontroller_pause_button_changed(bool paused);

  // 时间控制器播放速度变化
  void on_timecontroller_speed_changed(double speed);

  // 同步音频播放时间
  void on_music_pos_sync(double time);

 signals:
  // 时间戳更新信号
  void current_time_stamp_changed(double current_time_stamp);

  // 绝对bpm变化信号
  void current_absbpm_changed(double bpm);

  // 时间线速度变化信号
  void current_timeline_speed_changed(double timeline_speed);

  // 选中物件的信号
  void select_object(std::shared_ptr<Beat> beatinfo,
                     std::shared_ptr<HitObject> obj,
                     std::shared_ptr<Timing> ref_timing);
  // 是否暂停播放信号
  void pause_signal(bool paused);
};

#endif  // M_MAPWORKSPACE_H
