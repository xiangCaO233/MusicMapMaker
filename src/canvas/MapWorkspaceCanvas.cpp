#include "MapWorkspaceCanvas.h"

#include <qnamespace.h>
#include <qpaintdevice.h>
#include <qtmetamacros.h>

#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "../mmm/hitobject/Note/Hold.h"
#include "../mmm/hitobject/Note/HoldEnd.h"
#include "../mmm/hitobject/Note/Note.h"
#include "../mmm/map/osu/OsuMap.h"
#include "../mmm/timing/Timing.h"
#include "src/mmm/timing/osu/OsuTiming.h"

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
  auto unit = event->angleDelta().y() / 120 * timeline_zoom * height() / 20.0;
  current_time_stamp += unit * scroll_ratio;
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
  auto current_size = size();
  // 当前有绑定图
  // 读取获取当前时间附近的timings
  std::vector<std::shared_ptr<Timing>> temp_timings;
  working_map->query_around_timing(temp_timings, current_time_stamp);

  if (temp_timings.empty()) {
    // 未查询到timing-不绘制时间线
    return;
  }

  for (const auto &timing : temp_timings) {
    switch (timing->type) {
      case TimingType::OSUTIMING: {
        auto otiming = std::static_pointer_cast<OsuTiming>(timing);
        if (!otiming->is_inherit_timing) {
          // 非变速timing--存储的实际bpm
          current_abs_timing = otiming;
        } else {
          // 变速timing--存储的倍速
          speed_zoom = otiming->bpm;
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

  // 清除物件缓存
  buffer_objects.clear();
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

  // 当前处理到的时间
  int process_time = current_abs_timing->timestamp + beat_count * beattime;

  // 拍距离判定线距离从下往上--反转
  // 拍起始位置
  double distance = std::fabs(process_time - current_time_stamp);
  auto processing_pos = judgeline_pos + (distance * timeline_zoom);
  int32_t beat_index = 0;
  while (processing_pos > -beattime) {
    Beat beat(current_abs_timing->bpm, process_time, process_time + beattime,
              4);
    if (current_beats.size() > beat_index) {
      if (current_beats[beat_index] != beat) {
        // 不同,更新
        current_beats[beat_index] = beat;
      }
    } else {
      // 不足,添加
      current_beats.emplace_back(beat);
    }

    // from : process_time,to : process_time + beattime
    working_map->query_object_in_range(buffer_objects, process_time,
                                       process_time + beattime);

    process_time += beattime;
    processing_pos -= beat_distance;
    beat_index++;
  }
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
  auto texname = working_map->bg_path.filename().string();
  renderer_manager->texture_fillmode = TextureFillMode::SCALLING_AND_TILE;
  renderer_manager->addRect(des, texture_full_map[texname],
                            QColor(255, 255, 255, 255), 0, false);
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

  // TODO(xiang 2025-04-15): 绘制预览内容
  if (!working_map) {
  }

  // 绘制一层滤镜
  QRectF preview_area_bg_bound(current_size.width() * (1 - preview_width_scale),
                               0.0, current_size.width() * preview_width_scale,
                               current_size.height());
  renderer_manager->addRect(preview_area_bg_bound, nullptr, QColor(6, 6, 6, 75),
                            0, false);
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

// 绘制拍
void MapWorkspaceCanvas::draw_beats() {
  if (!working_map) return;

  auto current_size = size();
  for (const auto &beat : current_beats) {
    // 每拍时间*时间线缩放=拍距
    double beat_distance =
        60.0 / beat.bpm * 1000.0 * timeline_zoom * speed_zoom;

    // 分拍间距
    double divisor_distance = beat_distance / beat.divisors;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1.0 - judgeline_position);

    // 拍起始时间
    auto &beat_start_time = beat.start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        judgeline_pos -
        (beat_start_time - current_time_stamp) * timeline_zoom * speed_zoom;

    // 获取主题
    bool has_theme{false};
    std::vector<QColor> color_theme;
    auto color_theme_it = divisors_color_theme.find(beat.divisors);
    if (color_theme_it != divisors_color_theme.end()) {
      has_theme = true;
      color_theme = color_theme_it->second;
    }

    // 绘制小节线
    for (int i = 0; i < beat.divisors; i++) {
      // 小节线的位置
      double divisor_pos = beat_start_pos - i * divisor_distance;
      if (divisor_pos >= -beat_distance &&
          divisor_pos <= current_size.height() + beat_distance) {
        // 只绘制在可视范围内的小节线
        // 选择颜色
        QColor divisor_color;
        if (has_theme) {
          divisor_color = color_theme[i];
        } else {
          divisor_color = QColor(128, 128, 128, 240);
        }

        if (beat_start_time +
                i * (beat.end_timestamp - beat_start_time) / beat.divisors >=
            current_abs_timing->timestamp) {
          if (i == 0) {
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
        }
      }
    }
  }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject() {
  if (!working_map) return;

  auto current_size = size();
  // 声明+预分配内存
  std::vector<std::shared_ptr<Note>> temp_notes;
  std::vector<std::shared_ptr<Hold>> temp_holds;
  temp_notes.reserve(buffer_objects.size());
  temp_holds.reserve(buffer_objects.size());

  // 检查缓冲区,取出需要渲染的物件
  for (int i = 0; i < buffer_objects.size(); i++) {
    // 区分面尾物件
    if (buffer_objects[i]->is_hold_end) {
      // 找出面尾的头
      auto head =
          std::dynamic_pointer_cast<HoldEnd>(buffer_objects[i])->reference;
      // 判重(面头面尾同时在可视范围内)
      bool added{false};
      for (const auto &hold : temp_holds) {
        if (hold == head) {
          added = true;
        }
      }
      if (!added) {
        temp_holds.emplace_back(head);
        temp_notes.emplace_back(head);
      }
    } else {
      // 非面尾可转为Note
      auto note = std::dynamic_pointer_cast<Note>(buffer_objects[i]);
      // 直接使用
      if (note->type == NoteType::HOLD) {
        auto hold = std::dynamic_pointer_cast<Hold>(buffer_objects[i]);
        temp_holds.emplace_back(hold);
      }
      temp_notes.emplace_back(note);
    }
  }

  // 重新排序
  std::sort(temp_notes.begin(), temp_notes.end(),
            [](const std::shared_ptr<HitObject> &h1,
               const std::shared_ptr<HitObject> &h2) { return *h1 < *h2; });

  // TODO(xiang 2025-04-15): 执行渲染
  switch (working_map->maptype) {
    case MapType::OSUMAP: {
      // osu图
      auto omap = std::dynamic_pointer_cast<OsuMap>(working_map);
      int32_t max_orbit = omap->CircleSize;

      // 物件头的纹理
      auto &head_texture_pink = texture_full_map["Pink.png"];
      auto &head_texture_white = texture_full_map["White.png"];
      // 面条主体的纹理
      auto &long_note_body_texture_pink =
          texture_full_map["ArrowHoldBodyp.png"];
      auto &long_note_body_texture_white =
          texture_full_map["ArrowHoldBodyw.png"];
      // 面条尾的纹理
      auto &long_note_end_texture_pink = texture_full_map["ArrowHoldEndp.png"];
      auto &long_note_end_texture_white = texture_full_map["ArrowHoldEndw.png"];

      for (const auto &note : temp_notes) {
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
        // 判定线位置
        // auto judgeline_pos =
        //    current_size.height() * (1 - judgeline_position);
        auto head_note_size = QSizeF(head_texture->texture_image.width(),
                                     head_texture->texture_image.height());
        // 物件距离判定线距离从下往上--反转
        // 当前物件头位置-中心
        auto head_note_pos_y =
            current_size.height() * (1.0 - judgeline_position) -
            (note->timestamp - current_time_stamp) * timeline_zoom * speed_zoom;
        // 轨道宽度
        auto orbit_width =
            current_size.width() * (1 - preview_width_scale) / max_orbit;

        // 物件头中心位置
        auto note_center_pos = orbit_width * note->orbit + orbit_width / 2.0;
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
        switch (note->type) {
          case NoteType::NOTE: {
            // 直接绘制
            renderer_manager->addRect(head_note_bound, head_texture,
                                      QColor(0, 0, 0, 255), 0, true);
            break;
          }
          case NoteType::HOLD: {
            // 添加long_note_body
            auto long_note = std::dynamic_pointer_cast<Hold>(note);
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
                QSizeF(long_note_body_texture->texture_image.width(),
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
                QSizeF(long_note_end_texture->texture_image.width(),
                       long_note_end_texture->texture_image.height());
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

  draw_top_bar();

  draw_judgeline();
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> map) {
  working_map = map;

  // 加载背景图纹理
  add_texture(map->bg_path.string().c_str());

  // 重置谱面时间
  current_time_stamp = 0;
  emit current_time_stamp_changed(0);
}
