#include "MapWorkspaceCanvas.h"

#include <qlogging.h>
#include <qnamespace.h>
#include <qnumeric.h>
#include <qpaintdevice.h>
#include <qtimer.h>
#include <qtmetamacros.h>

#include <QDir>
#include <QKeyEvent>
#include <QThread>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../mmm/MapWorkProject.h"
#include "../mmm/hitobject/HitObject.h"
#include "../mmm/hitobject/Note/Hold.h"
#include "../mmm/hitobject/Note/HoldEnd.h"
#include "../mmm/hitobject/Note/Note.h"
#include "../mmm/map/osu/OsuMap.h"
#include "../mmm/map/rm/RMMap.h"
#include "../mmm/timing/Timing.h"
#include "../mmm/timing/osu/OsuTiming.h"
#include "../util/mutil.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "mmm/hitobject/Note/rm/Slide.h"

MapWorkspaceCanvas::MapWorkspaceCanvas(QWidget *parent) : GLCanvas(parent) {
  QColor white(255, 255, 255, 240);
  QColor red(255, 0, 0, 240);
  QColor purple(160, 102, 211);
  QColor blue(135, 206, 235);

  // 一分-白色
  auto &t1 = divisors_color_theme.try_emplace(1).first->second;
  t1.emplace_back(white);
  // 二分-白色/红色
  auto &t2 = divisors_color_theme.try_emplace(2).first->second;
  t2.emplace_back(white);
  t2.emplace_back(red);

  // 三分-白色/紫色/紫色
  auto &t3 = divisors_color_theme.try_emplace(3).first->second;
  t3.emplace_back(white);
  t3.emplace_back(purple);
  t3.emplace_back(purple);

  // 四分-白色/蓝色/红色/蓝色
  auto &t4 = divisors_color_theme.try_emplace(4).first->second;
  t4.emplace_back(white);
  t4.emplace_back(blue);
  t4.emplace_back(red);
  t4.emplace_back(blue);
}

MapWorkspaceCanvas::~MapWorkspaceCanvas() {};

void MapWorkspaceCanvas::initializeGL() {
  GLCanvas::initializeGL();
  // 初始化特效纹理序列
  // for (int i = 1; i <= 51; ++i) {
  //   auto name = "onhit/" + std::to_string(i) + ".png";
  //   auto &t = texture_full_map.at(name);
  //   hiteffects.emplace_back(t);
  // }
}

// 时间控制器暂停按钮触发
void MapWorkspaceCanvas::on_timecontroller_pause_button_changed(bool paused) {
  canvas_pasued = paused;
}

// 时间控制器播放速度变化
void MapWorkspaceCanvas::on_timecontroller_speed_changed(double speed) {
  playspeed = speed;
}

// qt事件
void MapWorkspaceCanvas::paintEvent(QPaintEvent *event) {
  GLCanvas::paintEvent(event);

  static long lasttime = 0;
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now().time_since_epoch())
                  .count();
  auto atime = time - lasttime;
  actual_update_time = double(atime) / 1000.0;
  if (actual_update_time - des_update_time > 1.5) {
    XWARN("qt update delayed:" + std::to_string(actual_update_time));
  }
  lasttime = time;

  if (!canvas_pasued) {
    // 未暂停,更新当前时间戳
    current_time_stamp = current_time_stamp + actual_update_time * playspeed;

    if (current_time_stamp < 0) {
      current_time_stamp = 0;
      canvas_pasued = true;
      emit pause_signal(canvas_pasued);
    }
    if (current_time_stamp > working_map->map_length) {
      current_time_stamp = working_map->map_length;
      canvas_pasued = true;
      emit pause_signal(canvas_pasued);
    }
    current_visual_time_stamp = current_time_stamp + static_time_offset;
    update_mapcanvas_timepos();
  }
}

// 更新fps显示
void MapWorkspaceCanvas::updateFpsDisplay(int fps) {
  QString title_suffix = QString(
                             "%1 FPS(glcalls: %2 | drawcalls: %3 | frametime: "
                             "%4 us | updatetime(qt): %5 ms)")
                             .arg(fps)
                             .arg(XLogger::glcalls)
                             .arg(XLogger::drawcalls)
                             .arg(pre_frame_time)
                             .arg(actual_update_time);
  emit update_window_title_suffix(title_suffix);
}

// 鼠标按下事件
void MapWorkspaceCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mousePressEvent(event);

  qDebug() << event->button();
  // 如果当前悬停的位置有物件,左键时选中此物件,右键时删除此物件(组合键头时删除组合键)
  // 没有物件则根据当前模式添加物件到鼠标位置对应的时间戳
  switch (event->button()) {
    case Qt::MouseButton::LeftButton: {
      mouse_left_pressed = true;
      mouse_left_press_pos = event->pos();
      if (select_bound_locate_points) {
        // 关闭选中框
        select_bound_locate_points = nullptr;
        select_bound.setWidth(0);
        select_bound.setHeight(0);
      }
      if (!(event->modifiers() & Qt::ControlModifier)) {
        // 未按住controll清空选中列表
        selected_hitobjects.clear();
      }
      // 按住controll左键多选
      if (hover_hitobject_info) {
        // 有悬浮在物件上
        selected_hitobjects.emplace(hover_hitobject_info->first);
      }
      break;
    }
    case Qt::MouseButton::RightButton: {
      break;
    }
  }
}

// 鼠标释放事件
void MapWorkspaceCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseReleaseEvent(event);
  switch (event->button()) {
    case Qt::MouseButton::LeftButton: {
      mouse_left_pressed = false;
    }
  }
}

// 鼠标双击事件
void MapWorkspaceCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void MapWorkspaceCanvas::mouseMoveEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseMoveEvent(event);

  // 更新鼠标操作区
  if (edit_area.contains(mouse_pos)) {
    operation_area = MouseOperationArea::EDIT;
  } else if (preview_area.contains(mouse_pos)) {
    operation_area = MouseOperationArea::PREVIEW;
  } else if (info_area.contains(mouse_pos)) {
    operation_area = MouseOperationArea::INFO;
  }

  if (mouse_left_pressed) {
    // 设置选中框定位点
    if (!select_bound_locate_points) {
      select_bound_locate_points =
          std::make_shared<std::pair<QPointF, QPointF>>(mouse_left_press_pos,
                                                        event->pos());
    } else {
      select_bound_locate_points->second = event->pos();
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
    if (!event->modifiers() & Qt::ControlModifier) {
      // 没按ctrl,先清空当前选中的
      selected_hitobjects.clear();
    }
  }
}

// 鼠标滚动事件
void MapWorkspaceCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  GLCanvas::wheelEvent(event);
  if (!working_map) return;

  // 修饰符
  auto modifiers = event->modifiers();

  // 编辑区
  double temp_scroll_ration{1.0};
  if (modifiers & Qt::ControlModifier) {
    // 在编辑区-按下controll滚动
    // 修改时间线缩放
    double res_timeline_zoom = timeline_zoom;
    if (event->angleDelta().y() > 0) {
      res_timeline_zoom += 0.05;
    } else {
      res_timeline_zoom -= 0.05;
    }
    if (res_timeline_zoom >= 0.5 && res_timeline_zoom <= 2.0) {
      timeline_zoom = res_timeline_zoom;
    }
    return;
  }
  if (modifiers & Qt::AltModifier) {
    // 在编辑区-按下alt滚动
    // 获取鼠标位置的拍--修改此拍分拍策略/改为自定义
  }
  if (modifiers & Qt::ShiftModifier) {
    // 在编辑区-按下shift滚动
    // 短暂增加滚动倍率
    temp_scroll_ration = 3.0;
  }
  if (magnet_to_divisor) {
    // 获取当前时间附近的拍
    // 找到第一个拍起始时间大于或等于当前时间的拍迭代器
    auto current_beat_it = working_map->beats.lower_bound(
        std::make_shared<Beat>(200, current_time_stamp, current_time_stamp));

    // TODO(xiang 2025-04-24): 修复吸附的bug
    // 待吸附列表
    std::vector<std::shared_ptr<Beat>> magnet_beats;

    // 区分分支
    if (current_beat_it == working_map->beats.end()) {
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
    } else if (current_beat_it == working_map->beats.begin()) {
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
      if (current_beat_it != working_map->beats.end()) {
        // auto next_beat = *current_beat_it;
        magnet_beats.emplace_back(*current_beat_it);
      }
    }
    // 待吸附时间集合
    std::set<double> divisor_times;
    // 初始化时间集合
    for (const auto &magnet_beat : magnet_beats) {
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
    if (event->angleDelta().y() * scroll_direction > 0) {
      // 向上吸附小节线
      if (std::abs(*current_divisor_it - current_time_stamp) < 2) {
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
      } else if (current_divisor_it == divisor_times.begin()) {
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
  } else {
    auto scroll_unit = (event->angleDelta().y() > 0 ? 1.0 : -1.0) *
                       timeline_zoom * height() / 10.0;
    current_time_stamp +=
        scroll_unit * temp_scroll_ration * scroll_ratio * scroll_direction;
  }
  if (current_time_stamp < 0) {
    current_time_stamp = 0;
    canvas_pasued = true;
    emit pause_signal(canvas_pasued);
  }
  if (current_time_stamp > working_map->map_length) {
    current_time_stamp = working_map->map_length;
    canvas_pasued = true;
    emit pause_signal(canvas_pasued);
  }
  current_visual_time_stamp = current_time_stamp + static_time_offset;
  update_mapcanvas_timepos();
}

// 键盘按下事件
void MapWorkspaceCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyPressEvent(event);

  // 捕获按键
  auto k = event->text();
  auto keycode = event->key();

  qDebug() << "code:" << keycode << ",text:" << k;

  switch (keycode) {
    case Qt::Key::Key_Space: {
      // 空格--播放与暂停
      if (!working_map) return;

      if (working_map->project_reference->devicename ==
          "unknown output device") {
        XWARN("未选择音频输出设备,无法切换暂停状态");
        return;
      }
      // 空格
      canvas_pasued = !canvas_pasued;
      emit pause_signal(canvas_pasued);
      break;
    }
    case Qt::Key_Delete: {
      // del键-删除
      break;
    }
  }

  if (keycode == 32) {
  }
}

// 键盘释放事件
void MapWorkspaceCanvas::keyReleaseEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyReleaseEvent(event);
}

// 取得焦点事件
void MapWorkspaceCanvas::focusInEvent(QFocusEvent *event) {
  // 传递事件
  GLCanvas::focusInEvent(event);
}

// 失去焦点事件
void MapWorkspaceCanvas::focusOutEvent(QFocusEvent *event) {
  // 传递事件
  GLCanvas::focusOutEvent(event);
}

// 更新谱面时间位置
void MapWorkspaceCanvas::update_mapcanvas_timepos() {
  // 更新项目中自己的位置
  working_map->project_reference->map_canvasposes.at(working_map) =
      current_time_stamp;
  emit current_time_stamp_changed(current_time_stamp);
}

// 调整尺寸事件
void MapWorkspaceCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  GLCanvas::resizeEvent(event);

  // 更新区域缓存
  // 信息区
  info_area.setX(0);
  info_area.setY(0);
  info_area.setWidth(width() * infoarea_width_scale);
  info_area.setHeight(height());

  // 编辑区
  edit_area.setX(0);
  edit_area.setY(0);
  edit_area.setWidth(width() * (1.0 - preview_width_scale));
  edit_area.setHeight(height());

  // 预览区
  preview_area.setX(width() * (1.0 - preview_width_scale));
  preview_area.setY(0);
  preview_area.setWidth(width() * preview_width_scale);
  edit_area.setHeight(height());
}

// 绘制背景
void MapWorkspaceCanvas::draw_background() {
  if (!working_map) return;
  auto des = QRectF(0, 0, width(), height());

  // 绘制背景图
  auto texname = working_map->bg_path.parent_path().filename().string() + "/" +
                 working_map->bg_path.filename().string();
  renderer_manager->texture_fillmode = TextureFillMode::SCALLING_AND_TILE;
  auto &t = texture_full_map[texname];
  renderer_manager->addRect(des, t, QColor(0, 0, 0, 255), 0, false);
  // 绘制背景遮罩
  if (background_darken_ratio != 0.0) {
    renderer_manager->addRect(
        des, nullptr, QColor(0, 0, 0, 255 * background_darken_ratio), 0, false);
  }
  // XINFO("bg_path:" + working_map->bg_path.string());
}

// 绘制顶部栏
void MapWorkspaceCanvas::draw_top_bar() {
  auto current_size = size();
  QRectF top_bar_out(0.0f, current_size.height() / 12.0f * -0.3f,
                     current_size.width(), current_size.height() / 12.0f);
  QRectF top_bar_in(
      current_size.width() / 48.0f,
      current_size.height() / 12.0f * -0.3f + current_size.height() / 48.0f,
      current_size.width() - (current_size.width() / 48.0f),
      current_size.height() / 12.0f - (current_size.height() / 48.0f));
  renderer_manager->addRoundRect(top_bar_in, nullptr, QColor(30, 40, 50, 230),
                                 0, 1.3, false);
  renderer_manager->addRoundRect(top_bar_out, nullptr, QColor(33, 33, 33, 230),
                                 0, 1.3, false);
  renderer_manager->addLine(QPointF(0, 0), QPointF(current_size.width(), 0),
                            2.0f, nullptr, QColor(255, 255, 255, 240), false);
}

// 绘制选中框
void MapWorkspaceCanvas::draw_select_bound() {
  if (select_bound_locate_points) {
    auto p1 = select_bound_locate_points->first;
    auto p2 = QPointF(select_bound_locate_points->first.x(),
                      select_bound_locate_points->second.y());
    auto p3 = select_bound_locate_points->second;
    auto p4 = QPointF(select_bound_locate_points->second.x(),
                      select_bound_locate_points->first.y());

    renderer_manager->addLine(p1, p2, 2, nullptr, QColor(255, 182, 193, 220),
                              true);
    renderer_manager->addLine(p2, p3, 2, nullptr, QColor(255, 182, 193, 220),
                              true);
    renderer_manager->addLine(p3, p4, 2, nullptr, QColor(255, 182, 193, 220),
                              true);
    renderer_manager->addLine(p1, p4, 2, nullptr, QColor(255, 182, 193, 220),
                              true);
  }
}

// 绘制预览
void MapWorkspaceCanvas::draw_preview_content() {
  auto current_size = size();
  auto preview_x_startpos = current_size.width() * (1 - preview_width_scale);

  // 绘制一层滤镜
  QRectF preview_area_bg_bound(preview_x_startpos, 0.0,
                               current_size.width() * preview_width_scale,
                               current_size.height());
  renderer_manager->addRect(preview_area_bg_bound, nullptr, QColor(6, 6, 6, 75),
                            0, false);

  // TODO(xiang 2025-04-15): 绘制预览内容
  if (working_map) {
    // 绘制整个map
    switch (working_map->maptype) {
      case MapType::OSUMAP: {
        auto omap = std::static_pointer_cast<OsuMap>(working_map);
        auto orbits = omap->CircleSize;
        size_t objindex = 0;

        // 获取预览区物件

        for (const auto &obj : buffer_preview_objects) {
          auto note = std::static_pointer_cast<Note>(obj);

          // 轨道宽度
          auto orbit_width =
              current_size.width() * preview_width_scale / orbits;

          // 最终纹理缩放倍率
          double texture_ratio{1.0};

          auto note_head_size = QSizeF(orbit_width * 0.8, 1.0);

          // 物件头中心位置
          auto note_center_pos_x = preview_x_startpos +
                                   orbit_width * note->orbit +
                                   orbit_width / 2.0;

          // 物件头左上角位置
          double head_note_pos_x =
              note_center_pos_x - note_head_size.width() / 2.0;

          double head_note_pos_y =
              current_size.height() *
                  (1.0 -
                   double(note->timestamp) / double(working_map->map_length)) -
              note_head_size.height() / 2.0;
          // qDebug() << head_note_pos_y;

          // 物件头的实际区域
          QRectF head_note_bound(
              head_note_pos_x, head_note_pos_y - note_head_size.height() / 2.0,
              note_head_size.width(), note_head_size.height());

          // 切换纹理绘制方式为填充
          renderer_manager->texture_fillmode = TextureFillMode::FILL;
          // 切换纹理绘制补齐方式为重采样
          renderer_manager->texture_complementmode =
              TextureComplementMode::REPEAT_TEXTURE;

          switch (obj->object_type) {
            case HitObjectType::OSUNOTE: {
              // 单物件
              // 直接绘制
              renderer_manager->addRoundRect(head_note_bound, nullptr,
                                             QColor(255, 182, 193, 255), 0, 1.3,
                                             false);
              break;
            }
            case HitObjectType::OSUHOLD: {
              break;
            }
            default:
              break;
          }
          ++objindex;
        }
        break;
      }
      case MapType::MALODYMAP: {
        break;
      }
      case MapType::RMMAP: {
        break;
      }
    }
    // 绘制当前位置--小框
    QSizeF preview_area_size(current_size.width() * preview_width_scale, 100);
    QRectF current_area(
        preview_x_startpos,
        current_size.height() * (1.0 - (current_visual_time_stamp /
                                        double(working_map->map_length))) -
            preview_area_size.height() / 2.0,
        preview_area_size.width(), preview_area_size.height());
    renderer_manager->addRect(current_area, nullptr, QColor(240, 240, 240, 75),
                              0, false);
  }
}
// 绘制判定线
void MapWorkspaceCanvas::draw_judgeline() {
  auto current_size = size();

  // 主区域判定线
  renderer_manager->addLine(
      QPointF(0, current_size.height() * (1.0 - judgeline_position)),
      QPointF(current_size.width() * (1 - preview_width_scale),
              current_size.height() * (1.0 - judgeline_position)),
      8, nullptr, QColor(0, 255, 255, 235), false);
  // 预览区域判定线
  renderer_manager->addLine(
      QPointF(current_size.width() * (1 - preview_width_scale),
              current_size.height() / 2.0),
      QPointF(current_size.width(), current_size.height() / 2.0), 6, nullptr,
      QColor(0, 255, 255, 235), false);
}

// 绘制信息区
void MapWorkspaceCanvas::draw_infoarea() {
  auto current_size = size();
  // 分隔线
  renderer_manager->addLine(
      QPointF(current_size.width() * infoarea_width_scale - 2, 0),
      QPointF(current_size.width() * infoarea_width_scale - 2,
              current_size.height()),
      4, nullptr, QColor(255, 182, 193, 235), false);

  // 标记timing
}

// 绘制拍
void MapWorkspaceCanvas::draw_beats() {
  if (!working_map) return;

  auto current_size = size();

  std::unordered_map<std::u32string, QPointF> timestr_rects;
  for (int i = 0; i < current_beats.size(); ++i) {
    auto &beat = current_beats[i];

    // 每拍时间*时间线缩放=拍距
    double beat_distance = 60.0 / beat->bpm * 1000.0 * timeline_zoom *
                           (canvas_pasued ? 1.0 : speed_zoom);

    // 分拍间距
    double divisor_distance = beat_distance / beat->divisors;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1.0 - judgeline_position);

    // 拍起始时间
    auto &beat_start_time = beat->start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        judgeline_pos - (beat_start_time - current_visual_time_stamp) *
                            timeline_zoom * (canvas_pasued ? 1.0 : speed_zoom);

    // 获取主题
    bool has_theme{false};
    std::vector<QColor> color_theme;
    auto color_theme_it = divisors_color_theme.find(beat->divisors);
    if (color_theme_it != divisors_color_theme.end()) {
      has_theme = true;
      color_theme = color_theme_it->second;
    }

    // 绘制小节线
    for (int j = 0; j < beat->divisors; ++j) {
      auto divisor_time =
          (beat->start_timestamp +
           (beat->end_timestamp - beat_start_time) / beat->divisors * j);
      // 筛选越界分拍线
      if (i < current_beats.size() - 1) {
        // 并非这波最后一拍
        // 取出下一拍引用
        auto &nextbeat = current_beats[i + 1];
        if (divisor_time > nextbeat->start_timestamp) {
          // 这个分拍线超过了下一拍的起始时间--跳过此分拍线的绘制
          continue;
        }
      }
      // 小节线的位置
      double divisor_pos = beat_start_pos - j * divisor_distance;
      if (divisor_pos >= -beat_distance &&
          divisor_pos <= current_size.height() + beat_distance) {
        // 只绘制在可视范围内的小节线
        // 选择颜色
        QColor divisor_color;
        if (has_theme) {
          divisor_color = color_theme[j];
        } else {
          divisor_color = QColor(128, 128, 128, 240);
        }

        // 过滤abstiming之前的小节线
        if (divisor_time >= current_abs_timing->timestamp) {
          if (j == 0) {
            // 小节线头画粗一点
            renderer_manager->addLine(
                QPointF(0, divisor_pos),
                QPointF(current_size.width() * (1 - preview_width_scale),
                        divisor_pos),
                6, nullptr, divisor_color, true);
          } else {
            renderer_manager->addLine(
                QPointF(0, divisor_pos),
                QPointF(current_size.width() * (1 - preview_width_scale),
                        divisor_pos),
                2, nullptr, divisor_color, true);
          }

          // 添加绘制精确时间
          // 计算精确时间--格式化
          std::u32string timestr;
          mutil::format_music_time2u32(timestr, divisor_time);
          timestr_rects.try_emplace(timestr).first->second =
              QPointF(4, divisor_pos - 4);
        }
      }
    }
  }
  // 一次全部绘制所有文本
  for (const auto &[str, pointf] : timestr_rects) {
    renderer_manager->addText(pointf, str, 16, "ComicShannsMono Nerd Font",
                              QColor(255, 182, 193, 240), 0, true);
  }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject() {
  if (!working_map) return;

  auto current_size = size();
  // 编辑区x起始位置
  auto edit_area_start_pos_x = current_size.width() * infoarea_width_scale;
  // 编辑区宽度
  auto edit_area_width =
      current_size.width() * (1.0 - infoarea_width_scale - preview_width_scale);

  // 轨道数
  int32_t max_orbit;

  // TODO(xiang 2025-04-15): 执行渲染
  switch (working_map->maptype) {
    case MapType::OSUMAP: {
      // osu图
      auto omap = std::dynamic_pointer_cast<OsuMap>(working_map);
      max_orbit = omap->CircleSize;
      break;
    }
    case MapType::RMMAP: {
      // rm图
      auto rmmap = std::dynamic_pointer_cast<RMMap>(working_map);
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

  // 物件头的纹理
  std::shared_ptr<TextureInstace> head_texture =
      texture_full_map["hitobject/head.png"];
  std::shared_ptr<TextureInstace> head_hovered_texture =
      texture_full_map["hitobject/head_hover.png"];
  std::shared_ptr<TextureInstace> head_selected_texture =
      texture_full_map["hitobject/head_selected.png"];

  // 面条主体的纹理
  std::shared_ptr<TextureInstace> long_note_body_vertical_texture =
      texture_full_map["hitobject/holdbodyvertical.png"];
  std::shared_ptr<TextureInstace> long_note_body_vertical_hovered_texture =
      texture_full_map["hitobject/holdbodyvertical_hover.png"];
  std::shared_ptr<TextureInstace> long_note_body_vertical_selected_texture =
      texture_full_map["hitobject/holdbodyvertical_selected.png"];

  // 横向面条主体的纹理
  std::shared_ptr<TextureInstace> long_note_body_horizontal_texture =
      texture_full_map["hitobject/holdbodyhorizontal.png"];
  std::shared_ptr<TextureInstace> long_note_body_horizontal_hovered_texture =
      texture_full_map["hitobject/holdbodyhorizontal_hover.png"];
  std::shared_ptr<TextureInstace> long_note_body_horizontal_selected_texture =
      texture_full_map["hitobject/holdbodyhorizontal_selected.png"];

  // 组合键节点纹理
  std::shared_ptr<TextureInstace> complex_note_node_texture =
      texture_full_map["hitobject/node.png"];
  std::shared_ptr<TextureInstace> complex_note_node_hovered_texture =
      texture_full_map["hitobject/node_hover.png"];
  std::shared_ptr<TextureInstace> complex_note_node_selected_texture =
      texture_full_map["hitobject/node_selected.png"];

  // 面条尾的纹理
  std::shared_ptr<TextureInstace> long_note_end_texture =
      texture_full_map["hitobject/holdend.png"];
  std::shared_ptr<TextureInstace> long_note_end_hovered_texture =
      texture_full_map["hitobject/holdend_hover.png"];
  std::shared_ptr<TextureInstace> long_note_end_selected_texture =
      texture_full_map["hitobject/holdend_selected.png"];

  // 滑键尾的纹理
  std::shared_ptr<TextureInstace> slide_left_note_end_texture =
      texture_full_map["hitobject/arrowleft.png"];
  std::shared_ptr<TextureInstace> slide_left_note_end_hovered_texture =
      texture_full_map["hitobject/arrowleft_hover.png"];
  std::shared_ptr<TextureInstace> slide_left_note_end_selected_texture =
      texture_full_map["hitobject/arrowleft_selected.png"];

  std::shared_ptr<TextureInstace> slide_right_note_end_texture =
      texture_full_map["hitobject/arrowright.png"];
  std::shared_ptr<TextureInstace> slide_right_note_end_hovered_texture =
      texture_full_map["hitobject/arrowright_hover.png"];
  std::shared_ptr<TextureInstace> slide_right_note_end_selected_texture =
      texture_full_map["hitobject/arrowright_selected.png"];

  // 轨道宽度
  auto orbit_width = edit_area_width / max_orbit;

  // 依据轨道宽度自动适应物件纹理尺寸
  // 物件尺寸缩放--相对于纹理尺寸
  auto width_scale = (orbit_width - 4.0) / double(head_texture->width);

  // 不大于1--不放大纹理
  double object_size_scale = std::min(width_scale, 1.0);

  auto head_note_size = QSizeF(head_texture->width * object_size_scale,
                               head_texture->height * object_size_scale);

  // 是否悬停在某一物件上
  bool is_hover_note{false};

  // 节点区域缓存
  std::map<std::shared_ptr<HitObject>, QRectF> nodes;

  // 防止重复绘制
  std::unordered_map<std::shared_ptr<HitObject>, bool> drawed_objects;

  // 渲染物件
  for (const auto &obj : buffer_objects) {
    if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX) continue;
    auto note = std::static_pointer_cast<Note>(obj);
    if (!note) continue;
    if (drawed_objects.find(note) == drawed_objects.end())
      drawed_objects.insert({note, true});
    else
      continue;

    double note_visual_time =
        current_visual_time_stamp +
        (note->timestamp - current_visual_time_stamp) * speed_zoom;

    // 物件距离判定线距离从下往上--反转
    // 当前物件头位置-中心
    auto note_center_pos_y =
        current_size.height() * (1.0 - judgeline_position) -
        ((canvas_pasued ? note->timestamp : note_visual_time) -
         current_visual_time_stamp) *
            timeline_zoom * (canvas_pasued ? 1.0 : speed_zoom);

    // 物件头中心位置
    auto note_center_pos_x =
        edit_area_start_pos_x + orbit_width * note->orbit + orbit_width / 2.0;

    // 打击特效帧
    // if (std::abs((canvas_pasued ? note->timestamp : note_visual_time) -
    //              current_time_stamp) < (des_update_time * 1.5) &&
    //     !canvas_pasued) {
    //   // 添加一序列的打击特效帧
    //   auto &framequeue = effects[note->orbit];
    //   auto w = hiteffects[0]->width * object_size_scale * 0.75;
    //   auto h = hiteffects[0]->height * object_size_scale * 0.75;
    //   for (const auto &hiteffect : hiteffects) {
    //     auto frame = HitEffectFrame();
    //     frame.effect_texture = hiteffect;
    //     frame.effect_bound.setX(note_center_pos_x - w / 2.0);
    //     frame.effect_bound.setY(
    //         current_size.height() * (1 - judgeline_position) - h / 2.0);
    //     frame.effect_bound.setWidth(w);
    //     frame.effect_bound.setHeight(h);
    //     framequeue.push(std::move(frame));
    //   }
    // }

    // 物件头左上角位置
    double head_note_pos_x = note_center_pos_x - head_note_size.width() / 2.0;

    // 物件头的实际区域
    QRectF head_note_bound(head_note_pos_x,
                           note_center_pos_y - head_note_size.height() / 2.0,
                           head_note_size.width(), head_note_size.height());

    // 横向身的高度
    auto horizon_body_height =
        long_note_body_horizontal_texture->height * object_size_scale;

    // 切换纹理绘制方式为填充
    renderer_manager->texture_fillmode = TextureFillMode::FILL;
    // 切换纹理绘制补齐方式为重采样
    renderer_manager->texture_complementmode =
        TextureComplementMode::REPEAT_TEXTURE;

    switch (note->note_type) {
      case NoteType::NOTE: {
        // 直接绘制
        if (head_note_bound.contains(mouse_pos)) {
          // 优先鼠标悬停时的绘制
          renderer_manager->addRect(head_note_bound, head_hovered_texture,
                                    QColor(0, 0, 0, 255), 0, true);
          hover_hitobject_info =
              std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                  note, true);
          is_hover_note = true;
        } else if (strict_select ? select_bound.contains(head_note_bound)
                                 : select_bound.intersects(head_note_bound) ||
                                       selected_hitobjects.find(note) !=
                                           selected_hitobjects.end()) {
          auto it = selected_hitobjects.find(note);
          if (it == selected_hitobjects.end()) {
            // 未选中则选中此物件
            selected_hitobjects.emplace(note);
          }
          // 选中时的绘制
          renderer_manager->addRect(head_note_bound, head_selected_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        } else {
          renderer_manager->addRect(head_note_bound, head_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        }
        break;
      }
      case NoteType::HOLD: {
        // 添加long_note_body
        auto long_note = std::dynamic_pointer_cast<Hold>(note);

        if (!long_note) continue;

        double long_note_end_visual_time =
            current_visual_time_stamp +
            (long_note->hold_end_reference->timestamp -
             current_visual_time_stamp) *
                speed_zoom;

        // 当前面条尾y轴位置
        auto long_note_end_pos_y =
            current_size.height() * (1.0 - judgeline_position) -
            ((canvas_pasued ? long_note->hold_end_reference->timestamp
                            : long_note_end_visual_time) -
             current_visual_time_stamp) *
                timeline_zoom * (canvas_pasued ? 1.0 : speed_zoom);
        auto long_note_body_height = (long_note_end_pos_y - note_center_pos_y);

        // 当前面条身中心位置,y位置偏下一个note
        auto long_note_body_pos_x = note_center_pos_x;
        auto long_note_body_pos_y =
            note_center_pos_y + (long_note_end_pos_y - note_center_pos_y) / 2.0;

        // 面身实际尺寸高度-0.5note
        auto long_note_body_size =
            QSizeF(long_note_body_vertical_texture->width * object_size_scale,
                   long_note_body_height - 0.5 * head_note_bound.height());

        // 面身的实际区域--
        QRectF long_note_body_bound(
            long_note_body_pos_x - long_note_body_size.width() / 2.0,
            long_note_body_pos_y - long_note_body_size.height() / 2.0,
            long_note_body_size.width(), long_note_body_size.height());

        // 添加long_note_end
        // 当前面条尾中心位置
        auto long_note_end_pos_x = note_center_pos_x;
        // 面尾实际尺寸
        auto long_note_end_size =
            QSizeF(long_note_end_texture->width * object_size_scale,
                   long_note_end_texture->height * object_size_scale);
        // 面尾的实际区域--在面身缺的那一note位置
        QRectF long_note_end_bound(
            long_note_end_pos_x - long_note_end_size.width() / 2.0,
            long_note_end_pos_y - long_note_end_size.height() / 2.0,
            long_note_end_size.width(), long_note_end_size.height());
        // 先绘制body
        if (long_note_body_bound.contains(mouse_pos)) {
          renderer_manager->addRect(long_note_body_bound,
                                    long_note_body_vertical_hovered_texture,
                                    QColor(0, 0, 0, 255), 0, true);
          hover_hitobject_info =
              std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                  note, false);
          is_hover_note = true;
        } else if (strict_select
                       ? select_bound.contains(long_note_body_bound)
                       : select_bound.intersects(long_note_body_bound) ||
                             selected_hitobjects.find(note) !=
                                 selected_hitobjects.end()) {
          // 未选中则选中此物件
          selected_hitobjects.emplace(note);
          renderer_manager->addRect(long_note_body_bound,
                                    long_note_body_vertical_selected_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        } else {
          renderer_manager->addRect(long_note_body_bound,
                                    long_note_body_vertical_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        }

        // 节点尺寸
        auto node_size =
            QSizeF(complex_note_node_texture->width * object_size_scale,
                   complex_note_node_texture->height * object_size_scale);
        // 节点的实际区域--也在面身缺的那note位置
        QRectF node_bound(long_note_end_pos_x - node_size.width() / 2.0,
                          long_note_end_pos_y - node_size.height() / 2.0,
                          node_size.width(), node_size.height());

        switch (long_note->compinfo) {
          case ComplexInfo::NONE: {
            // 头尾都画
            if (long_note_end_bound.contains(mouse_pos)) {
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      note, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(long_note_end_bound)
                           : select_bound.intersects(long_note_end_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 未选中则选中此面尾
              selected_hitobjects.emplace(long_note->hold_end_reference);
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }

            if (head_note_bound.contains(mouse_pos)) {
              renderer_manager->addRect(head_note_bound, head_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      long_note->hold_end_reference, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(head_note_bound)
                           : select_bound.intersects(head_note_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 未选中则选中此物件
              selected_hitobjects.emplace(note);
              // 选中时的绘制
              renderer_manager->addRect(head_note_bound, head_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(head_note_bound, head_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            break;
          }
            //  组合键中的子键
          case ComplexInfo::HEAD: {
            // 多画个头-在尾处追加一个节点
            if (head_note_bound.contains(mouse_pos)) {
              renderer_manager->addRect(head_note_bound, head_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      note, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(head_note_bound)
                           : select_bound.intersects(head_note_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 未选中则选中此物件
              selected_hitobjects.emplace(note);
              // 选中时的绘制
              renderer_manager->addRect(head_note_bound, head_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(head_note_bound, head_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            nodes.try_emplace(note, node_bound);
            break;
          }
          case ComplexInfo::BODY: {
            // 仅在尾处追加一个节点
            nodes.try_emplace(note, node_bound);

            break;
          }
          case ComplexInfo::END: {
            // 补齐节点,画个尾
            for (const auto &[notereference, noderect] : nodes) {
              if (noderect.contains(mouse_pos)) {
                renderer_manager->addRect(noderect,
                                          complex_note_node_hovered_texture,
                                          QColor(0, 0, 0, 255), 0, true);
                // 组合键的节点(节点是相当于那一物件的尾)
                hover_hitobject_info = std::make_shared<
                    std::pair<std::shared_ptr<HitObject>, bool>>(notereference,
                                                                 true);
                is_hover_note = true;
              } else if (strict_select
                             ? select_bound.contains(noderect)
                             : select_bound.intersects(noderect) ||
                                   selected_hitobjects.find(notereference) !=
                                       selected_hitobjects.end()) {
                selected_hitobjects.emplace(notereference);
                renderer_manager->addRect(noderect,
                                          complex_note_node_selected_texture,
                                          QColor(0, 0, 0, 255), 0, true);
              } else {
                renderer_manager->addRect(noderect, complex_note_node_texture,
                                          QColor(0, 0, 0, 255), 0, true);
              }
            }

            if (long_note_end_bound.contains(mouse_pos)) {
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      long_note->hold_end_reference, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(long_note_end_bound)
                           : select_bound.intersects(long_note_end_bound) ||
                                 selected_hitobjects.find(
                                     long_note->hold_end_reference) !=
                                     selected_hitobjects.end()) {
              // 选中此面尾
              selected_hitobjects.emplace(long_note->hold_end_reference);
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(long_note_end_bound,
                                        long_note_end_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            // 并清空节点缓存
            nodes.clear();
            break;
          }
        }
        break;
      }
      case NoteType::SLIDE: {
        // 滑键
        auto slide = std::static_pointer_cast<Slide>(note);
        if (!slide) continue;
        auto endorbit = slide->orbit + slide->slide_parameter;

        // 横向身的终点位置
        auto horizon_body_end_pos_x =
            edit_area_start_pos_x + orbit_width * endorbit + orbit_width / 2.0;

        // 横向身的尺寸
        // 横向身的宽度
        auto horizon_body_width =
            std::abs(horizon_body_end_pos_x - note_center_pos_x) +
            horizon_body_height;

        // 滑尾纹理
        std::shared_ptr<TextureInstace> slide_end_texture;
        std::shared_ptr<TextureInstace> slide_end_hovered_texture;
        std::shared_ptr<TextureInstace> slide_end_selected_texture;

        // 横向身的具体位置
        double horizon_body_pos_x = -horizon_body_height / 2.0;
        if (slide->slide_parameter > 0) {
          // 右滑,矩形就在头物件中心位置
          horizon_body_pos_x += note_center_pos_x;
          slide_end_texture = slide_right_note_end_texture;
          slide_end_hovered_texture = slide_right_note_end_hovered_texture;
          slide_end_selected_texture = slide_right_note_end_selected_texture;
        } else {
          // 左滑,矩形整体左移矩形宽度的位置
          horizon_body_pos_x = horizon_body_pos_x + note_center_pos_x -
                               horizon_body_width + horizon_body_height;
          slide_end_texture = slide_left_note_end_texture;
          slide_end_hovered_texture = slide_left_note_end_hovered_texture;
          slide_end_selected_texture = slide_left_note_end_selected_texture;
        }
        auto horizon_body_bound = QRectF(
            horizon_body_pos_x, note_center_pos_y - horizon_body_height / 2.0,
            horizon_body_width, horizon_body_height);

        // 箭头位置--滑键结束轨道的位置
        auto slide_end_bound =
            QRectF(horizon_body_end_pos_x -
                       slide_end_texture->width * object_size_scale / 2.0,
                   note_center_pos_y -
                       slide_end_texture->height * object_size_scale / 2.0,
                   slide_end_texture->width * object_size_scale,
                   slide_end_texture->height * object_size_scale);

        // 先绘制横向身,然后头和箭头
        if (horizon_body_bound.contains(mouse_pos)) {
          renderer_manager->addRect(horizon_body_bound,
                                    long_note_body_horizontal_hovered_texture,
                                    QColor(0, 0, 0, 255), 0, true);
          hover_hitobject_info =
              std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                  note, false);
          is_hover_note = true;
        } else if (strict_select
                       ? select_bound.contains(horizon_body_bound)
                       : select_bound.intersects(horizon_body_bound) ||
                             selected_hitobjects.find(note) !=
                                 selected_hitobjects.end()) {
          // 选中此物件
          selected_hitobjects.emplace(note);
          renderer_manager->addRect(horizon_body_bound,
                                    long_note_body_horizontal_selected_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        } else {
          renderer_manager->addRect(horizon_body_bound,
                                    long_note_body_horizontal_texture,
                                    QColor(0, 0, 0, 255), 0, true);
        }

        // 节点尺寸
        auto node_size =
            QSizeF(complex_note_node_texture->width * object_size_scale,
                   complex_note_node_texture->height * object_size_scale);
        // 节点的实际区域--也在面身缺的那note位置
        QRectF node_bound(horizon_body_end_pos_x - node_size.width() / 2.0,
                          note_center_pos_y - node_size.height() / 2.0,
                          node_size.width(), node_size.height());

        switch (slide->compinfo) {
          case ComplexInfo::NONE: {
            // 头尾都画
            if (head_note_bound.contains(mouse_pos)) {
              renderer_manager->addRect(head_note_bound, head_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      note, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(head_note_bound)
                           : select_bound.intersects(head_note_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 选中此物件
              selected_hitobjects.emplace(note);
              // 选中时的绘制
              renderer_manager->addRect(head_note_bound, head_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(head_note_bound, head_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }

            if (slide_end_bound.contains(mouse_pos)) {
              renderer_manager->addRect(slide_end_bound,
                                        slide_end_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      slide->slide_end_reference, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(slide_end_bound)
                           : select_bound.intersects(slide_end_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 选中此物件
              selected_hitobjects.emplace(note);
              renderer_manager->addRect(slide_end_bound,
                                        slide_end_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(slide_end_bound, slide_end_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            break;
          }
          case ComplexInfo::HEAD: {
            // 多画个头-在尾处追加一个节点
            if (head_note_bound.contains(mouse_pos)) {
              renderer_manager->addRect(head_note_bound, head_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      note, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(head_note_bound)
                           : select_bound.intersects(head_note_bound) ||
                                 selected_hitobjects.find(note) !=
                                     selected_hitobjects.end()) {
              // 选中此物件
              selected_hitobjects.emplace(note);
              // 选中时的绘制
              renderer_manager->addRect(head_note_bound, head_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(head_note_bound, head_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            nodes.try_emplace(note, node_bound);
            break;
          }
          case ComplexInfo::BODY: {
            // 仅在尾处追加一个节点
            nodes.try_emplace(note, node_bound);
            break;
          }
          case ComplexInfo::END: {
            // 补齐节点,画个尾
            for (const auto &[notereference, noderect] : nodes) {
              if (noderect.contains(mouse_pos)) {
                renderer_manager->addRect(noderect,
                                          complex_note_node_hovered_texture,
                                          QColor(0, 0, 0, 255), 0, true);
                // 组合键的节点(节点是相当于那一物件的尾)
                hover_hitobject_info = std::make_shared<
                    std::pair<std::shared_ptr<HitObject>, bool>>(notereference,
                                                                 true);
                is_hover_note = true;
              } else if (strict_select
                             ? select_bound.contains(noderect)
                             : select_bound.intersects(noderect) ||
                                   selected_hitobjects.find(notereference) !=
                                       selected_hitobjects.end()) {
                selected_hitobjects.emplace(notereference);
                renderer_manager->addRect(noderect,
                                          complex_note_node_selected_texture,
                                          QColor(0, 0, 0, 255), 0, true);
              } else {
                renderer_manager->addRect(noderect, complex_note_node_texture,
                                          QColor(0, 0, 0, 255), 0, true);
              }
            }

            if (slide_end_bound.contains(mouse_pos)) {
              renderer_manager->addRect(slide_end_bound,
                                        slide_end_hovered_texture,
                                        QColor(0, 0, 0, 255), 0, true);
              hover_hitobject_info =
                  std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
                      slide->slide_end_reference, true);
              is_hover_note = true;
            } else if (strict_select
                           ? select_bound.contains(slide_end_bound)
                           : select_bound.intersects(slide_end_bound) ||
                                 selected_hitobjects.find(
                                     slide->slide_end_reference) !=
                                     selected_hitobjects.end()) {
              selected_hitobjects.emplace(slide->slide_end_reference);
              renderer_manager->addRect(slide_end_bound,
                                        slide_end_selected_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            } else {
              renderer_manager->addRect(slide_end_bound, slide_end_texture,
                                        QColor(0, 0, 0, 255), 0, true);
            }
            // 并清空节点缓存
            nodes.clear();
            break;
          }
        }
        break;
      }
    }
  }
  if (!is_hover_note) {
    // 未悬浮在任何一个物件或物件身体上
    hover_hitobject_info = nullptr;
  }
  // 最后给每个轨道添加特效动画
  // for (auto &[orbit, framequeue] : effects) {
  //   if (!framequeue.empty()) {
  //     renderer_manager->addRect(framequeue.front().effect_bound,
  //                               framequeue.front().effect_texture,
  //                               QColor(0, 0, 0, 255), 0, true);
  //     framequeue.pop();
  //   }
  // }
}

// 渲染实际图形
void MapWorkspaceCanvas::push_shape() {
  if (working_map) {
    // 当前有绑定图
    auto current_size = size();
    // 读取获取当前时间附近的timings
    std::vector<std::shared_ptr<Timing>> temp_timings;
    working_map->query_around_timing(temp_timings, current_visual_time_stamp);

    if (temp_timings.empty()) {
      // 未查询到timing-不绘制时间线
      return;
    }

    // 更新参考绝对timing
    for (const auto &timing : temp_timings) {
      switch (timing->type) {
        case TimingType::OSUTIMING: {
          auto otiming = std::static_pointer_cast<OsuTiming>(timing);
          if (!otiming->is_inherit_timing) {
            // 非变速timing--存储的实际bpm
            current_abs_timing = otiming;
            if (!preference_bpm) {
              // 只更新一次参考bpm
              preference_bpm = std::make_unique<double>();
              *preference_bpm = current_abs_timing->basebpm;
            }
            speed_zoom = current_abs_timing->basebpm / *preference_bpm;
            emit current_absbpm_changed(current_abs_timing->basebpm);
            emit current_timeline_speed_changed(speed_zoom);
          } else {
            // 变速timing--存储的倍速
            speed_zoom =
                current_abs_timing->basebpm / *preference_bpm * otiming->bpm;
            emit current_timeline_speed_changed(speed_zoom);
          }
          break;
        }
        case TimingType::RMTIMING: {
          // rmtiming只能存bpm
          auto rmtiming = std::static_pointer_cast<Timing>(timing);
          current_abs_timing = rmtiming;
          speed_zoom = 1.0;
          emit current_absbpm_changed(current_abs_timing->basebpm);
          emit current_timeline_speed_changed(speed_zoom);
          break;
        }
        case TimingType::MALODYTIMING: {
          break;
        }
        case TimingType::UNKNOWN:
          return;
      }
    }

    // 使用timing计算时间
    // 每拍时间
    auto beattime = 60.0 / current_abs_timing->bpm * 1000.0;
    // 每拍时间*时间线缩放=拍距
    double beat_distance = beattime * timeline_zoom * speed_zoom;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1.0 - judgeline_position);

    // 距离此timing的拍数-1
    auto beat_count = int(
        (current_visual_time_stamp - current_abs_timing->timestamp) / beattime -
        1);

    // TODO(xiang 2025-04-21):
    // 精确计算获取需要绘制的拍--保证不多不少(算入时间线缩放,变速缩放)
    // 当前处理的时间范围--大致
    current_time_area_start =
        current_abs_timing->timestamp + beat_count * beattime;
    current_time_area_end =
        current_abs_timing->timestamp + beat_count * beattime;

    // 拍距离判定线距离从下往上--反转
    // 拍起始位置
    double distance =
        std::fabs(current_time_area_end - current_visual_time_stamp);
    auto processing_pos = judgeline_pos + (distance * timeline_zoom);

    while (processing_pos > -beattime) {
      current_time_area_end += beattime;
      processing_pos -= beat_distance;
    }

    draw_background();

    if (canvas_pasued) {
      // 更新拍列表
      // 清除拍缓存
      current_beats.clear();
      working_map->query_beat_in_range(current_beats,
                                       int32_t(current_time_area_start),
                                       int32_t(current_time_area_end));
      draw_beats();
    }

    // 更新物件列表
    // 清除物件缓存
    buffer_objects.clear();
    working_map->query_object_in_range(buffer_objects,
                                       int32_t(current_time_area_start),
                                       int32_t(current_time_area_end), true);
    draw_hitobject();
  }
  draw_select_bound();
  draw_preview_content();

  draw_judgeline();
  draw_infoarea();

  draw_top_bar();
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> map) {
  // 此处确保了未打开项目是无法switchmap的(没有选项)
  working_map = map;
  preference_bpm = nullptr;
  canvas_pasued = true;

  if (map) {
    map_type = map->maptype;
    auto s = QDir(map->audio_file_abs_path);
    // 更新谱面长度(如果音乐比谱面长)
    auto map_audio_length =
        BackgroundAudio::get_audio_length(s.canonicalPath().toStdString());
    XINFO("maplength:" + std::to_string(map->map_length));
    XINFO("audiolength:" + std::to_string(map_audio_length));
    if (map->map_length < map_audio_length) map->map_length = map_audio_length;
    // 设置谱面时间
    current_visual_time_stamp = map->project_reference->map_canvasposes.at(map);

    // 加载背景图纹理
    add_texture(map->bg_path);
  } else {
    current_visual_time_stamp = 0;
  }

  emit pause_signal(canvas_pasued);
  emit current_time_stamp_changed(current_visual_time_stamp);
}
