#include "MapWorkspaceCanvas.h"

#include <qnamespace.h>
#include <qpaintdevice.h>
#include <qtmetamacros.h>

#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#include "../mmm/hitobject/Note/Hold.h"
#include "../mmm/hitobject/Note/HoldEnd.h"
#include "../mmm/hitobject/Note/Note.h"
#include "../mmm/map/osu/OsuMap.h"
#include "../mmm/timing/Timing.h"
#include "mmm/hitobject/HitObject.h"
#include "src/mmm/timing/osu/OsuTiming.h"
#include "util/mutil.h"

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

  // 初始化定时器--绑定updatecanvas函数
  refresh_timer = new QTimer(this);
  QObject::connect(refresh_timer, &QTimer::timeout,
                   [this]() { this->update_canvas(); });
  refresh_timer->start(timer_update_time);
}

MapWorkspaceCanvas::~MapWorkspaceCanvas() = default;

// qt事件
// 鼠标按下事件
void MapWorkspaceCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mousePressEvent(event);
}

// 鼠标释放事件
void MapWorkspaceCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseReleaseEvent(event);
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
      } else if (current_time_stamp > last_beat->start_timestamp &&
                 current_time_stamp < last_beat->end_timestamp) {
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
    auto unit = event->angleDelta().y() / 120 * timeline_zoom * height() / 10.0;
    current_time_stamp +=
        unit * temp_scroll_ration * scroll_ratio * scroll_direction;
  }

  emit current_time_stamp_changed(current_time_stamp);
}

// 键盘按下事件
void MapWorkspaceCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyPressEvent(event);
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

// 进入事件
void MapWorkspaceCanvas::enterEvent(QEnterEvent *event) {
  // 传递事件
  GLCanvas::enterEvent(event);
}

// 退出事件
void MapWorkspaceCanvas::leaveEvent(QEvent *event) {
  // 传递事件
  GLCanvas::leaveEvent(event);
}

// 调整尺寸事件
void MapWorkspaceCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  GLCanvas::resizeEvent(event);

  // 更新区域缓存
  edit_area.setX(0);
  edit_area.setY(0);
  edit_area.setWidth(width() * (1.0 - preview_width_scale));
  edit_area.setHeight(height());

  preview_area.setX(width() * (1.0 - preview_width_scale));
  preview_area.setY(0);
  preview_area.setWidth(width() * preview_width_scale);
  edit_area.setHeight(height());
}

// 更新画布
void MapWorkspaceCanvas::update_canvas() {
  if (!working_map) return;
  // 当前有绑定图
  auto current_size = size();

  // 读取获取当前时间附近的timings
  std::vector<std::shared_ptr<Timing>> temp_timings;
  working_map->query_around_timing(temp_timings, current_time_stamp);

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
          speed_zoom = 1.0;
          emit current_absbpm_changed(current_abs_timing->basebpm);
          emit current_timeline_speed_changed(speed_zoom);
        } else {
          // 变速timing--存储的倍速
          speed_zoom = otiming->bpm;
          emit current_timeline_speed_changed(speed_zoom);
        }
        break;
      }
      case TimingType::RMTIMING: {
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
  auto beat_count =
      int((current_time_stamp - current_abs_timing->timestamp) / beattime - 1);

  // TODO(xiang 2025-04-21):
  // 精确计算获取需要绘制的拍--保证不多不少(算入时间线缩放,变速缩放)
  // 当前处理的时间范围--大致
  current_time_area_start =
      current_abs_timing->timestamp + beat_count * beattime;
  current_time_area_end = current_abs_timing->timestamp + beat_count * beattime;

  // 拍距离判定线距离从下往上--反转
  // 拍起始位置
  double distance = std::fabs(current_time_area_end - current_time_stamp);
  auto processing_pos = judgeline_pos + (distance * timeline_zoom);

  while (processing_pos > -beattime) {
    current_time_area_end += beattime;
    processing_pos -= beat_distance;
  }

  // 更新拍列表
  // 清除拍缓存
  current_beats.clear();
  working_map->query_beat_in_range(current_beats,
                                   int32_t(current_time_area_start),
                                   int32_t(current_time_area_end));
  // 更新物件列表
  // 清除物件缓存
  buffer_objects.clear();
  working_map->query_object_in_range(buffer_objects,
                                     int32_t(current_time_area_start),
                                     int32_t(current_time_area_end), true);

  if (!pasue) {
    // 未暂停,更新当前时间戳
    current_time_stamp += timer_update_time;
  }

  draw_background();
  draw_beats();
  draw_hitobject();

  repaint();
}

// 绘制背景
void MapWorkspaceCanvas::draw_background() {
  if (!working_map) return;
  auto des = QRectF(0, 0, width(), height());

  // 绘制背景图
  auto texname = working_map->bg_path.parent_path().filename().string() + "/" +
                 working_map->bg_path.filename().string();
  renderer_manager->texture_fillmode = TextureFillMode::SCALLING_AND_TILE;
  renderer_manager->addRect(des, texture_full_map[texname],
                            QColor(255, 255, 255, 255), 0, false);
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
        current_size.height() *
                (1.0 - (current_time_stamp / double(working_map->map_length))) -
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
  for (int i = 0; i < current_beats.size(); ++i) {
    auto &beat = current_beats[i];
    // 每拍时间*时间线缩放=拍距
    double beat_distance =
        60.0 / beat->bpm * 1000.0 * timeline_zoom * speed_zoom;

    // 分拍间距
    double divisor_distance = beat_distance / beat->divisors;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1.0 - judgeline_position);

    // 拍起始时间
    auto &beat_start_time = beat->start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        judgeline_pos -
        (beat_start_time - current_time_stamp) * timeline_zoom * speed_zoom;

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

          // 绘制精确时间
          // 计算精确时间--格式化
          std::u32string timestr;
          mutil::format_music_time2u32(timestr, divisor_time);
          renderer_manager->addText(QPointF(4, divisor_pos - 4), timestr, 16,
                                    "ComicShannsMono Nerd Font",
                                    QColor(255, 182, 193, 240), 0);
        }
      }
    }
  }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject() {
  if (!working_map) return;

  auto current_size = size();
  auto edit_area_start_pos_x = current_size.width() * infoarea_width_scale;
  auto edit_area_width =
      current_size.width() * (1.0 - infoarea_width_scale - preview_width_scale);

  // TODO(xiang 2025-04-15): 执行渲染
  switch (working_map->maptype) {
    case MapType::OSUMAP: {
      // osu图
      auto omap = std::dynamic_pointer_cast<OsuMap>(working_map);
      int32_t max_orbit = omap->CircleSize;

      // 物件头的纹理
      auto &head_texture_pink = texture_full_map["hitobject/Pink.png"];
      auto &head_texture_white = texture_full_map["hitobject/White.png"];
      // 面条主体的纹理
      auto &long_note_body_texture_pink =
          texture_full_map["hitobject/ArrowHoldBodyp.png"];
      auto &long_note_body_texture_white =
          texture_full_map["hitobject/ArrowHoldBodyw.png"];
      // 面条尾的纹理
      auto &long_note_end_texture_pink =
          texture_full_map["hitobject/ArrowHoldEndp.png"];
      auto &long_note_end_texture_white =
          texture_full_map["hitobject/ArrowHoldEndw.png"];

      for (const auto &obj : buffer_objects) {
        auto note = std::static_pointer_cast<Note>(obj);
        // 粉白
        std::shared_ptr<TextureInstace> head_texture;
        std::shared_ptr<TextureInstace> long_note_body_texture;
        std::shared_ptr<TextureInstace> long_note_end_texture;
        if (max_orbit == 4) {
          // 4k--粉白白粉
          if (note->orbit == 0 || note->orbit == 3) {
            head_texture = head_texture_pink;
            long_note_body_texture = long_note_body_texture_pink;
            long_note_end_texture = long_note_end_texture_pink;
          } else {
            head_texture = head_texture_white;
            long_note_body_texture = long_note_body_texture_white;
            long_note_end_texture = long_note_end_texture_white;
          }
        } else if (max_orbit == 6) {
          // 6k--白粉白白粉白
          if (note->orbit == 1 || note->orbit == 4) {
            head_texture = head_texture_pink;
            long_note_body_texture = long_note_body_texture_pink;
            long_note_end_texture = long_note_end_texture_pink;
          } else {
            head_texture = head_texture_white;
            long_note_body_texture = long_note_body_texture_white;
            long_note_end_texture = long_note_end_texture_white;
          }
        } else {
          if (note->orbit % 2 == 0) {
            head_texture = head_texture_pink;
            long_note_body_texture = long_note_body_texture_pink;
            long_note_end_texture = long_note_end_texture_pink;
          } else {
            head_texture = head_texture_white;
            long_note_body_texture = long_note_body_texture_white;
            long_note_end_texture = long_note_end_texture_white;
          }
        }
        // 轨道宽度
        auto orbit_width = edit_area_width / max_orbit;

        // 依据轨道宽度自动适应物件纹理尺寸
        // 物件尺寸缩放--相对于纹理尺寸
        auto width_scale =
            (orbit_width - 4.0) / double(head_texture->texture_image.width());

        // 不大于1--不放大纹理
        double object_size_scale = std::min(width_scale, 1.0);

        auto head_note_size =
            QSizeF(head_texture->texture_image.width() * object_size_scale,
                   head_texture->texture_image.height() * object_size_scale);

        // 物件距离判定线距离从下往上--反转
        // 当前物件头位置-中心
        auto head_note_pos_y =
            current_size.height() * (1.0 - judgeline_position) -
            (note->timestamp - current_time_stamp) * timeline_zoom * speed_zoom;

        // 物件头中心位置
        auto note_center_pos = edit_area_start_pos_x +
                               orbit_width * note->orbit + orbit_width / 2.0;
        // 物件头左上角位置
        double head_note_pos_x = note_center_pos - head_note_size.width() / 2.0;

        // 物件头的实际区域
        QRectF head_note_bound(head_note_pos_x,
                               head_note_pos_y - head_note_size.height() / 2.0,
                               head_note_size.width(), head_note_size.height());

        // 切换纹理绘制方式为填充
        renderer_manager->texture_fillmode = TextureFillMode::FILL;
        // 切换纹理绘制补齐方式为重采样
        renderer_manager->texture_complementmode =
            TextureComplementMode::REPEAT_TEXTURE;
        switch (note->note_type) {
          case NoteType::NOTE: {
            // 直接绘制
            renderer_manager->addRect(head_note_bound, head_texture,
                                      QColor(0, 0, 0, 255), 0, true);
            break;
          }
          case NoteType::HOLD: {
            // 添加long_note_body
            auto long_note = std::dynamic_pointer_cast<Hold>(note);
            if (!long_note) continue;
            // 当前面条尾y轴位置
            auto long_note_end_pos_y =
                current_size.height() * (1.0 - judgeline_position) -
                (long_note->hold_end_reference->timestamp -
                 current_time_stamp) *
                    timeline_zoom * speed_zoom;
            // 当前面条身中心位置,y位置偏下一个note
            auto long_note_body_pos_x = note_center_pos;
            auto long_note_body_pos_y =
                head_note_pos_y + (long_note_end_pos_y - head_note_pos_y) / 2.0;

            // 面身实际尺寸高度-1.5note
            auto long_note_body_size =
                QSizeF(long_note_body_texture->texture_image.width() *
                           object_size_scale,
                       long_note_end_pos_y - head_note_pos_y -
                           0.5 * head_note_bound.height());

            // 面身的实际区域--
            QRectF long_note_body_bound(
                long_note_body_pos_x - long_note_body_size.width() / 2.0,
                long_note_body_pos_y - long_note_body_size.height() / 2.0,
                long_note_body_size.width(), long_note_body_size.height());

            // 添加long_note_end
            // 当前面条尾中心位置
            auto long_note_end_pos_x = note_center_pos;
            // 面尾实际尺寸
            auto long_note_end_size =
                QSizeF(long_note_end_texture->texture_image.width() *
                           object_size_scale,
                       long_note_end_texture->texture_image.height() *
                           object_size_scale);
            // 面尾的实际区域--在面身缺的那一note位置
            QRectF long_note_end_bound(
                long_note_end_pos_x - long_note_end_size.width() / 2.0,
                long_note_end_pos_y - long_note_end_size.height() / 2.0,
                long_note_end_size.width(), long_note_end_size.height());
            // 先绘制body再绘制end,最后head
            renderer_manager->addRect(long_note_body_bound,
                                      long_note_body_texture,
                                      QColor(0, 0, 0, 255), 0, true);
            renderer_manager->addRect(long_note_end_bound,
                                      long_note_end_texture,
                                      QColor(0, 0, 0, 255), 0, true);
            renderer_manager->addRect(head_note_bound, head_texture,
                                      QColor(0, 0, 0, 255), 0, true);
            break;
          }
        }
      }
      break;
    }
    case MapType::RMMAP: {
      // rm图
      break;
    }
    case MapType::MALODYMAP: {
      // ma图
      break;
    }
    default:
      break;
  }
}

// 渲染实际图形
void MapWorkspaceCanvas::push_shape() {
  draw_preview_content();

  draw_judgeline();
  draw_infoarea();

  draw_top_bar();
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> map) {
  working_map = map;
  map_type = map->maptype;

  if (map) {
    // 加载背景图纹理
    add_texture(map->bg_path);
  }

  // 重置谱面时间
  current_time_stamp = 0;
  emit current_time_stamp_changed(0);
}
