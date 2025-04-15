#include "MapWorkspaceCanvas.h"

#include <qnamespace.h>
#include <qpaintdevice.h>

#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "../mmm/hitobject/Note/Hold.h"
#include "../mmm/hitobject/Note/HoldEnd.h"
#include "../mmm/hitobject/Note/Note.h"
#include "../mmm/timing/Timing.h"

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

  // 初始化默认纹理
  load_texture_from_path("../resources/textures/default/hitobject",
                         TexturePoolType::ARRAY, true);

  // 初始化定时器
  refresh_timer = new QTimer(this);
  // Lambda 捕获 this,调用成员函数
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
}

// 鼠标滚动事件
void MapWorkspaceCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  GLCanvas::wheelEvent(event);
  current_time_stamp += event->pixelDelta().y() / 3;
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
}
// 更新画布
void MapWorkspaceCanvas::update_canvas() {
  // 重绘更新
  auto current_size = size();
  if (working_map) {
    // 当前有绑定图
    // TODO(xiang 2025-04-15): 更新可视拍列表
    // 读取timing
    Timing target_timing;
    target_timing.timestamp = current_time_stamp;
    if (current_timing_list) {
      if (current_timing_list->empty()) {
        // 不存在timing时,不绘制时间线
        return;
      } else {
        auto it = std::upper_bound(
            current_timing_list->begin(), current_timing_list->end(),
            target_timing,
            [&](Timing target, const std::shared_ptr<Timing> &ptr) {
              return target < *ptr;
            });
        if (it == current_timing_list->begin()) {
          // 取第一个
          target_timing = *(current_timing_list->at(0));
        } else if (it == current_timing_list->end()) {
          // 取最后一个
          target_timing =
              *(current_timing_list->at(current_timing_list->size() - 1));
        } else {
          target_timing = **(it - 1);
        }
      }
    } else {
      // 未初始化timing表,不绘制时间线
      return;
    }
    // 清除物件缓存
    buffer_objects.clear();
    // 使用timing计算时间
    // 每拍时间
    auto beattime = 60 / target_timing.bpm * 1000;
    // 每拍时间*时间线缩放=拍距
    double beat_distance = beattime * timeline_zoom;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1 - judgeline_position);

    // 距离此timing的拍数-1
    auto beat_count =
        int((current_time_stamp - target_timing.timestamp) / beattime - 1);

    // 当前处理到的时间
    int process_time = target_timing.timestamp + beat_count * beattime;

    // 拍距离判定线距离从下往上--反转
    // 拍起始位置
    double distance = std::fabs(process_time - current_time_stamp);
    auto start_pos = judgeline_pos + (distance * timeline_zoom);
    int32_t start_beat_index = 0;
    while (start_pos > -beattime) {
      Beat beat(target_timing.bpm, process_time, process_time + beattime, 4);
      if (current_beats.size() > start_beat_index) {
        if (current_beats[start_beat_index] != beat) {
          // 不同,更新
          current_beats[start_beat_index] = beat;
        }
      } else {
        // 不足,添加
        current_beats.emplace_back(beat);
      }
      // TODO(xiang 2025-04-15): 更新当前拍可视物件列表
      // from : process_time,to : process_time + beattime
      std::shared_ptr<HitObject> temp_startnote =
          std::make_shared<Note>((uint32_t)(process_time));
      auto startit = std::upper_bound(
          working_map->hitobjects.begin(), working_map->hitobjects.end(),
          temp_startnote,
          [](const std::shared_ptr<HitObject> &ho1,
             const std::shared_ptr<HitObject> &ho2) { return *ho1 < *ho2; });
      if (startit == working_map->hitobjects.begin()) {
        // 所有元素 > target
        startit = working_map->hitobjects.end();
      } else {
        // 使用上一个(比目标小)
        startit--;
      }
      std::shared_ptr<HitObject> temp_endnote =
          std::make_shared<Note>((uint32_t)(process_time + beattime));
      auto endit = std::upper_bound(
          working_map->hitobjects.begin(), working_map->hitobjects.end(),
          temp_endnote,
          [](const std::shared_ptr<HitObject> &ho1,
             const std::shared_ptr<HitObject> &ho2) { return *ho1 < *ho2; });
      if (endit == working_map->hitobjects.begin()) {
        // 所有元素 > target
        startit = working_map->hitobjects.end();
      } else {
        // 使用当前这个(比目标大)
      }
      if (startit < endit)
        // 合法区间--将次区间所有物件添加到缓存列表
        for (; startit < endit; startit++)
          buffer_objects.emplace_back(*startit);

      process_time += beattime;
      start_pos -= beat_distance;
      start_beat_index++;
    }
  }
  if (!pasue) {
    // 未暂停,更新当前时间戳
    current_time_stamp += timer_update_time;
  }
  update();
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
                                 0, 0.3, false);
  renderer_manager->addRoundRect(top_bar_out, nullptr, QColor(33, 33, 33, 230),
                                 0, 0.3, false);
  renderer_manager->addLine(QPointF(0, 0), QPointF(current_size.width(), 0),
                            2.0f, nullptr, QColor(255, 255, 255, 240), false);
}

// 绘制预览
void MapWorkspaceCanvas::draw_preview_content() {
  // TODO(xiang 2025-04-15): 绘制预览内容
}
// 绘制判定线
void MapWorkspaceCanvas::draw_judgeline() {
  auto current_size = size();
  renderer_manager->addLine(
      QPointF(0, current_size.height() * (1 - judgeline_position)),
      QPointF(current_size.width(),
              current_size.height() * (1 - judgeline_position)),
      8, nullptr, QColor(0, 255, 255, 235), false);
}

// 绘制拍
void MapWorkspaceCanvas::draw_beats() {
  auto current_size = size();
  for (const auto &beat : current_beats) {
    // 每拍时间*时间线缩放=拍距
    double beat_distance = 60 / beat.bpm * 1000 * timeline_zoom;

    // 分拍间距
    double divisor_distance = beat_distance / beat.divisors;

    // 判定线位置
    auto judgeline_pos = current_size.height() * (1 - judgeline_position);

    // 拍起始时间
    auto &beat_start_time = beat.start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        // pre:4659 - 5120
        judgeline_pos - (beat_start_time - current_time_stamp) * timeline_zoom;

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

        renderer_manager->addLine(QPointF(0, divisor_pos),
                                  QPointF(current_size.width(), divisor_pos), 2,
                                  nullptr, divisor_color, true);
      }
    }
  }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject() {
  // 声明+预分配内存
  std::vector<std::shared_ptr<HitObject>> temp_hitobjects;
  std::vector<std::shared_ptr<Hold>> temp_holds;
  temp_hitobjects.reserve(buffer_objects.size());
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
        temp_hitobjects.emplace_back(head);
      }
    } else {
      // 非面尾可转为Note
      auto note = std::dynamic_pointer_cast<Note>(buffer_objects[i]);
      // 直接使用
      if (note->type == NoteType::HOLD) {
        auto hold = std::dynamic_pointer_cast<Hold>(buffer_objects[i]);
        temp_holds.emplace_back(hold);
      }
      temp_hitobjects.emplace_back(note);
    }
  }
  // TODO(xiang 2025-04-15): 执行渲染
}

// 渲染实际图形
void MapWorkspaceCanvas::push_shape() {
  draw_top_bar();
  draw_preview_content();
  draw_judgeline();

  draw_beats();
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> map) {
  working_map = map;
  current_timing_list = &map->timings;
  current_time_stamp = 5000;
}
