#include "MapWorkspaceCanvas.h"

#include <qcolor.h>
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
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../log/colorful-log.h"
#include "../../mmm/MapWorkProject.h"
#include "../../mmm/hitobject/HitObject.h"
#include "../../mmm/hitobject/Note/Note.h"
#include "../../mmm/hitobject/Note/rm/Slide.h"
#include "../audio/BackgroundAudio.h"
#include "MapWorkspaceSkin.h"
#include "editor/MapEditor.h"
#include "generator/general/HoldGenerator.h"
#include "generator/general/NoteGenerator.h"
#include "generator/general/SlideGenerator.h"

MapWorkspaceCanvas::MapWorkspaceCanvas(QWidget *parent)
    : skin(this), GLCanvas(parent) {
  // 初始化编辑器
  editor = std::make_shared<MapEditor>(this);
  // 注册物件生成器
  // 单物件
  objgenerators[NoteType::NOTE] = std::make_shared<NoteGenerator>(editor);
  // 面条物件
  objgenerators[NoteType::HOLD] = std::make_shared<HoldGenerator>(editor);
  // 滑键物件
  objgenerators[NoteType::SLIDE] = std::make_shared<SlideGenerator>(editor);

  // 初始化节拍生成器
  beatgenerator = std::make_shared<BeatGenerator>(editor);

  // 初始化时间区域信息生成器
  areagenerator = std::make_shared<AreaInfoGenerator>(editor);
}

MapWorkspaceCanvas::~MapWorkspaceCanvas() {};

void MapWorkspaceCanvas::initializeGL() {
  GLCanvas::initializeGL();
  // 初始化默认皮肤
  auto default_skin_path =
      std::filesystem::path("../resources/textures/default");
  skin.load_skin(default_skin_path);
}

// 时间控制器暂停按钮触发
void MapWorkspaceCanvas::on_timecontroller_pause_button_changed(bool paused) {
  editor->canvas_pasued = paused;
}

// 时间控制器播放速度变化
void MapWorkspaceCanvas::on_timecontroller_speed_changed(double speed) {
  editor->playspeed = speed;
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

  if (!editor->canvas_pasued) {
    // 未暂停,更新当前时间戳
    editor->current_time_stamp =
        editor->current_time_stamp + actual_update_time * editor->playspeed;

    if (editor->current_time_stamp < 0) {
      editor->current_time_stamp = 0;
      editor->canvas_pasued = true;
      emit pause_signal(editor->canvas_pasued);
    }
    if (editor->current_time_stamp > working_map->map_length) {
      editor->current_time_stamp = working_map->map_length;
      editor->canvas_pasued = true;
      emit pause_signal(editor->canvas_pasued);
    }
    editor->current_visual_time_stamp =
        editor->current_time_stamp + editor->static_time_offset;
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
      editor->mouse_left_pressed = true;
      editor->mouse_left_press_pos = event->pos();
      if (editor->select_bound_locate_points) {
        // 关闭选中框
        editor->select_bound_locate_points = nullptr;
        editor->select_bound.setWidth(0);
        editor->select_bound.setHeight(0);
      }
      if (!(event->modifiers() & Qt::ControlModifier)) {
        // 未按住controll清空选中列表
        editor->selected_hitobjects.clear();
      }
      // 按住controll左键多选
      if (editor->hover_hitobject_info) {
        // 有悬浮在物件上
        editor->selected_hitobjects.emplace(
            editor->hover_hitobject_info->first);
      }

      // 发送更新选中物件信号
      if (editor->hover_hitobject_info) {
        emit select_object(editor->hover_hitobject_info->second,
                           editor->hover_hitobject_info->first,
                           editor->current_abs_timing);
      } else {
        emit select_object(nullptr, nullptr, nullptr);
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
      editor->mouse_left_pressed = false;
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
  if (editor->edit_area.contains(mouse_pos)) {
    editor->operation_area = MouseOperationArea::EDIT;
  } else if (editor->preview_area.contains(mouse_pos)) {
    editor->operation_area = MouseOperationArea::PREVIEW;
  } else if (editor->info_area.contains(mouse_pos)) {
    editor->operation_area = MouseOperationArea::INFO;
  }

  if (editor->mouse_left_pressed) {
    // 设置选中框定位点
    if (!editor->select_bound_locate_points) {
      editor->select_bound_locate_points =
          std::make_shared<std::pair<QPointF, QPointF>>(
              editor->mouse_left_press_pos, event->pos());
    } else {
      editor->select_bound_locate_points->second = event->pos();
    }

    // 更新选中区域
    editor->select_bound.setX(
        std::min(editor->select_bound_locate_points->first.x(),
                 editor->select_bound_locate_points->second.x()));
    editor->select_bound.setY(
        std::min(editor->select_bound_locate_points->first.y(),
                 editor->select_bound_locate_points->second.y()));
    editor->select_bound.setWidth(
        std::abs(editor->select_bound_locate_points->first.x() -
                 editor->select_bound_locate_points->second.x()));
    editor->select_bound.setHeight(
        std::abs(editor->select_bound_locate_points->first.y() -
                 editor->select_bound_locate_points->second.y()));

    // 更新选中的物件内容
    if (!event->modifiers() & Qt::ControlModifier) {
      // 没按ctrl,先清空当前选中的
      editor->selected_hitobjects.clear();
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
    double res_timeline_zoom = editor->timeline_zoom;
    if (event->angleDelta().y() > 0) {
      res_timeline_zoom += 0.05;
    } else {
      res_timeline_zoom -= 0.05;
    }
    if (res_timeline_zoom >= 0.5 && res_timeline_zoom <= 2.0) {
      editor->timeline_zoom = res_timeline_zoom;
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
  if (editor->magnet_to_divisor) {
    // 获取当前时间附近的拍
    // 找到第一个拍起始时间大于或等于当前时间的拍迭代器
    auto current_beat_it =
        working_map->beats.lower_bound(std::make_shared<Beat>(
            200, editor->current_time_stamp, editor->current_time_stamp));

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

      if (editor->current_time_stamp == last_beat->start_timestamp) {
        // 在拍头--添加倒数第二拍的小节线和本拍小节线
        current_beat_it--;
        // auto second_tolast_beat = *current_beat_it;
        magnet_beats.emplace_back(*current_beat_it);
      } else if (editor->current_visual_time_stamp >
                     last_beat->start_timestamp &&
                 editor->current_visual_time_stamp < last_beat->end_timestamp) {
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
    auto current_divisor_it =
        divisor_times.lower_bound(editor->current_time_stamp);
    // 根据滑动方向选择吸附哪一个
    if (event->angleDelta().y() * editor->scroll_direction > 0) {
      // 向上吸附小节线
      if (std::abs(*current_divisor_it - editor->current_time_stamp) < 2) {
        // 防止非法访问最后一个元素后的元素
        auto lasttime = divisor_times.end();
        lasttime--;
        if (current_divisor_it != lasttime) {
          // 找到自己了-吸下一个
          ++current_divisor_it;
          editor->current_time_stamp = *current_divisor_it;
        } else {
          // 不动
        }
      } else if (current_divisor_it == divisor_times.begin()) {
        // 选择的大于自己的第一个
        editor->current_time_stamp = *current_divisor_it;
      }
    } else {
      // 防止非法访问第一个元素前的元素
      if (current_divisor_it != divisor_times.begin()) {
        // 向下吸附小节线--不管是找到自己还是找到下一个,吸上一个
        --current_divisor_it;
        editor->current_time_stamp = *current_divisor_it;
      }
    }
  } else {
    auto scroll_unit = (event->angleDelta().y() > 0 ? 1.0 : -1.0) *
                       editor->timeline_zoom * height() / 10.0;
    editor->current_time_stamp += scroll_unit * temp_scroll_ration *
                                  editor->scroll_ratio *
                                  editor->scroll_direction;
  }
  if (editor->current_time_stamp < 0) {
    editor->current_time_stamp = 0;
    editor->canvas_pasued = true;
    emit pause_signal(editor->canvas_pasued);
  }
  if (editor->current_time_stamp > working_map->map_length) {
    editor->current_time_stamp = working_map->map_length;
    editor->canvas_pasued = true;
    emit pause_signal(editor->canvas_pasued);
  }
  editor->current_visual_time_stamp =
      editor->current_time_stamp + editor->static_time_offset;
  played_effects_objects.clear();
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
      editor->canvas_pasued = !editor->canvas_pasued;
      emit pause_signal(editor->canvas_pasued);
      break;
    }
    case Qt::Key_Delete: {
      // del键-删除
      break;
    }
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
      editor->current_time_stamp;
  emit current_time_stamp_changed(editor->current_time_stamp);
}

// 调整尺寸事件
void MapWorkspaceCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  GLCanvas::resizeEvent(event);

  // 更新区域缓存
  // 信息区
  editor->info_area.setX(0);
  editor->info_area.setY(0);
  editor->info_area.setWidth(width() * editor->infoarea_width_scale);
  editor->info_area.setHeight(height());

  // 编辑区
  editor->edit_area.setX(0);
  editor->edit_area.setY(0);
  editor->edit_area.setWidth(width() * (1.0 - editor->preview_width_scale));
  editor->edit_area.setHeight(height());

  editor->update_size(size());

  // 预览区
  editor->preview_area.setX(width() * (1.0 - editor->preview_width_scale));
  editor->preview_area.setY(0);
  editor->preview_area.setWidth(width() * editor->preview_width_scale);
  editor->edit_area.setHeight(height());
}

// 绘制背景
void MapWorkspaceCanvas::draw_background() {
  if (!working_map) return;
  auto des = QRectF(0, 0, width(), height());

  // 绘制背景图
  renderer_manager->texture_fillmode = TextureFillMode::SCALLING_AND_TILE;
  auto &t =
      texture_full_map[QString::fromStdString(working_map->bg_path.string())
                           .toStdString()];
  renderer_manager->addRect(des, t, QColor(0, 0, 0, 255), 0, false);
  // 绘制背景遮罩
  if (editor->background_darken_ratio != 0.0) {
    renderer_manager->addRect(
        des, nullptr, QColor(0, 0, 0, 255 * editor->background_darken_ratio), 0,
        false);
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
  if (editor->select_bound_locate_points) {
    auto &border_left_texture =
        skin.get_selected_border_texture(SelectBorderDirection::LEFT);
    auto &border_right_texture =
        skin.get_selected_border_texture(SelectBorderDirection::RIGHT);
    auto &border_top_texture =
        skin.get_selected_border_texture(SelectBorderDirection::TOP);
    auto &border_bottom_texture =
        skin.get_selected_border_texture(SelectBorderDirection::BOTTOM);

    auto p1 = editor->select_bound_locate_points->first;
    auto p2 = QPointF(editor->select_bound_locate_points->first.x(),
                      editor->select_bound_locate_points->second.y());
    auto p3 = editor->select_bound_locate_points->second;
    auto p4 = QPointF(editor->select_bound_locate_points->second.x(),
                      editor->select_bound_locate_points->first.y());

    // 左矩形p1-p2
    auto leftrect =
        QRectF(p1.x() - editor->select_border_width / 2.0,
               p1.y() < p2.y() ? p1.y() - editor->select_border_width / 2.0
                               : p2.y() - editor->select_border_width / 2.0,
               editor->select_border_width,
               std::abs(p2.y() - p1.y()) + editor->select_border_width);

    // 右矩形p3-p4
    auto rightrect =
        QRectF(p4.x() - editor->select_border_width / 2.0,
               p4.y() < p3.y() ? p4.y() - editor->select_border_width / 2.0
                               : p3.y() - editor->select_border_width / 2.0,
               editor->select_border_width,
               std::abs(p3.y() - p4.y()) + editor->select_border_width);

    // 上矩形p1-p4
    auto toprect =
        QRectF(p1.x() < p4.x() ? p1.x() - editor->select_border_width / 2.0
                               : p4.x() - editor->select_border_width / 2.0,
               p1.y() - editor->select_border_width / 2.0,
               std::abs(p4.x() - p1.x()) + editor->select_border_width,
               editor->select_border_width);

    // 下矩形p2-p3
    auto bottomrect =
        QRectF(p2.x() < p3.x() ? p2.x() - editor->select_border_width / 2.0
                               : p3.x() - editor->select_border_width / 2.0,
               p2.y() - editor->select_border_width / 2.0,
               std::abs(p3.x() - p2.x()) + editor->select_border_width,
               editor->select_border_width);

    renderer_manager->addRect(leftrect, border_left_texture,
                              QColor(0, 0, 0, 255), 0, true);
    renderer_manager->addRect(rightrect, border_right_texture,
                              QColor(0, 0, 0, 255), 0, true);
    renderer_manager->addRect(toprect, border_top_texture, QColor(0, 0, 0, 255),
                              0, true);
    renderer_manager->addRect(bottomrect, border_bottom_texture,
                              QColor(0, 0, 0, 255), 0, true);
  }
}

// 绘制预览
void MapWorkspaceCanvas::draw_preview_content() {
  auto current_size = size();
  auto preview_x_startpos =
      current_size.width() * (1 - editor->preview_width_scale);

  // 绘制一层滤镜
  QRectF preview_area_bg_bound(
      preview_x_startpos, 0.0,
      current_size.width() * editor->preview_width_scale,
      current_size.height());
  renderer_manager->addRect(preview_area_bg_bound, nullptr, QColor(6, 6, 6, 75),
                            0, false);
}

// 绘制判定线
void MapWorkspaceCanvas::draw_judgeline() {
  auto current_size = size();

  // 主区域判定线
  renderer_manager->addLine(
      QPointF(0, current_size.height() * (1.0 - editor->judgeline_position)),
      QPointF(current_size.width() * (1 - editor->preview_width_scale),
              current_size.height() * (1.0 - editor->judgeline_position)),
      8, nullptr, QColor(0, 255, 255, 235), false);
  // 预览区域判定线
  renderer_manager->addLine(
      QPointF(current_size.width() * (1 - editor->preview_width_scale),
              current_size.height() / 2.0),
      QPointF(current_size.width(), current_size.height() / 2.0), 6, nullptr,
      QColor(0, 255, 255, 235), false);
}

// 绘制信息区
void MapWorkspaceCanvas::draw_infoarea() {
  auto current_size = size();
  // 分隔线
  renderer_manager->addLine(
      QPointF(current_size.width() * editor->infoarea_width_scale - 2, 0),
      QPointF(current_size.width() * editor->infoarea_width_scale - 2,
              current_size.height()),
      4, nullptr, QColor(255, 182, 193, 235), false);

  // 标记timing
}

// 绘制拍
void MapWorkspaceCanvas::draw_beats() {
  if (!working_map) return;
  // 生成图形数据
  beatgenerator->generate();

  // 先绘制所有节拍线
  while (!BeatGenerator::line_queue.empty()) {
    auto &line = BeatGenerator::line_queue.front();
    renderer_manager->addLine(
        QPointF(line.x1, line.y1), QPointF(line.x2, line.y2), line.line_width,
        nullptr, QColor(line.r, line.g, line.b, line.a), true);

    BeatGenerator::line_queue.pop();
  }

  // 绘制所有时间字符串
  while (!BeatGenerator::text_queue.empty()) {
    auto &text = BeatGenerator::text_queue.front();
    renderer_manager->addText(QPointF(text.x, text.y), text.text,
                              skin.timeinfo_font_size, skin.font_family,
                              skin.timeinfo_font_color, 0, true);
    BeatGenerator::text_queue.pop();
  }
}

// 播放特效
void MapWorkspaceCanvas::play_effect(double xpos, double ypos,
                                     int32_t frame_count, EffectType etype) {
  std::shared_ptr<TextureInstace> effect_frame_texture;
  switch (etype) {
    case EffectType::NORMAL: {
      // 视觉
      effect_frame_texture =
          texture_full_map[skin.nomal_hit_effect_dir + "/1.png"];
      for (int i = 1; i <= frame_count; ++i) {
        auto w = effect_frame_texture->width * (editor->object_size_scale);
        auto h = effect_frame_texture->height * (editor->object_size_scale);
        effect_frame_queue_map[xpos].emplace(
            QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
            texture_full_map[skin.nomal_hit_effect_dir + "/" +
                             std::to_string(i % 30 + 1) + ".png"]);
      }
      // 听觉
      BackgroundAudio::play_audio(
          working_map->project_reference->devicename,
          skin.get_sound_effect(SoundEffectType::COMMON_HIT), 0);
      break;
    }
    case EffectType::SLIDEARROW: {
      // 视觉
      effect_frame_texture =
          texture_full_map[skin.slide_hit_effect_dir + "/1.png"];
      for (int i = 1; i <= frame_count; ++i) {
        auto w = effect_frame_texture->width * (editor->object_size_scale);
        auto h = effect_frame_texture->height * (editor->object_size_scale);
        effect_frame_queue_map[xpos].emplace(
            QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
            texture_full_map[skin.slide_hit_effect_dir + "/" +
                             std::to_string(i % 16 + 1) + ".png"]);
      }
      // 听觉
      BackgroundAudio::play_audio(working_map->project_reference->devicename,
                                  skin.get_sound_effect(SoundEffectType::SLIDE),
                                  0);
      break;
    }
  }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject() {
  if (!working_map) return;
  auto current_size = size();
  // 防止重复绘制
  std::unordered_map<std::shared_ptr<HitObject>, bool> drawed_objects;

  // 渲染物件
  // new
  // 计算图形
  for (const auto &obj : editor->buffer_objects) {
    if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX) continue;
    auto note = std::static_pointer_cast<Note>(obj);
    if (!note) continue;
    if (drawed_objects.find(note) == drawed_objects.end())
      drawed_objects.insert({note, true});
    else
      continue;
    // 启动播放线程
    std::thread playthread;
    // 物件的横向偏移
    auto x = editor->edit_area_start_pos_x +
             editor->orbit_width * std::static_pointer_cast<Note>(obj)->orbit +
             editor->orbit_width / 2.0;
    // 轨道宽度
    auto ow = editor->orbit_width;
    // 画布引用
    auto canvas = this;
    if (!played_effects_objects.contains(obj)) {
      // 经过判定线
      if (std::abs(obj->timestamp - editor->current_visual_time_stamp) <
          des_update_time * 2) {
        // 播放物件添加到的位置
        auto obj_it = played_effects_objects.emplace(obj).first;

        // 播放线程
        playthread = std::thread([=]() {
          auto play_x = x;
          // 特效类型
          EffectType t;
          // 特效帧数
          int32_t frames;
          if (note) {
            switch (note->note_type) {
              case NoteType::SLIDE: {
                if (note->compinfo != ComplexInfo::NONE &&
                    note->compinfo != ComplexInfo::END)
                  return;

                t = EffectType::SLIDEARROW;
                play_x +=
                    (std::static_pointer_cast<Slide>(note))->slide_parameter *
                    ow;
                frames = 16 * (1 / canvas->editor->playspeed);
                break;
              }
              default: {
                t = EffectType::NORMAL;
                frames = 30 * (1 / canvas->editor->playspeed);
                break;
              }
            }
            if (note->note_type == NoteType::HOLD) {
              auto hold = std::static_pointer_cast<Hold>(note);
              frames = hold->hold_time / canvas->des_update_time * 2 *
                       (1 / canvas->editor->playspeed);
            }

            canvas->play_effect(play_x,
                                canvas->size().height() *
                                    (1 - canvas->editor->judgeline_position),
                                frames, t);
          }
        });
        playthread.detach();
      }
    }
    if (note) {
      // 生成物件
      const auto &generator = objgenerators[note->note_type];
      generator->generate(obj);
      if (note->note_type == NoteType::NOTE) {
        // 除单键外的物件已经处理了头
        generator->object_enqueue();
      }
    }
  }

  // 切换纹理绘制方式为填充
  renderer_manager->texture_fillmode = TextureFillMode::FILL;
  // 切换纹理绘制补齐方式为重采样
  renderer_manager->texture_complementmode =
      TextureComplementMode::REPEAT_TEXTURE;
  // 按计算层级渲染图形
  while (!ObjectGenerator::shape_queue.empty()) {
    auto &shape = ObjectGenerator::shape_queue.front();
    if (shape.is_over_current_time) {
      renderer_manager->texture_effect = TextureEffect::HALF_TRANSPARENT;
    }

    if (!editor->canvas_pasued && shape.objref &&
        shape.objref->timestamp <= editor->current_time_stamp) {
      // 播放中且过了判定线时间使用半透明效果
      renderer_manager->texture_effect = TextureEffect::HALF_TRANSPARENT;
    }
    renderer_manager->addRect(QRectF(shape.x, shape.y, shape.w, shape.h),
                              shape.tex, QColor(0, 0, 0, 255), 0, true);
    renderer_manager->texture_effect = TextureEffect::NONE;
    ObjectGenerator::shape_queue.pop();
  }

  // 更新hover信息
  if (!editor->is_hover_note) {
    // 未悬浮在任何一个物件或物件身体上
    editor->hover_hitobject_info = nullptr;
  } else {
    editor->is_hover_note = false;
  }
}

// 渲染实际图形
void MapWorkspaceCanvas::push_shape() {
  // 绘制背景
  draw_background();
  if (working_map) {
    // 生成区域信息
    areagenerator->generate();
    if (editor->canvas_pasued) draw_beats();

    // 更新物件列表
    // 清除物件缓存
    editor->buffer_objects.clear();
    working_map->query_object_in_range(
        editor->buffer_objects, int32_t(editor->current_time_area_start),
        int32_t(editor->current_time_area_end), true);
    draw_hitobject();
    renderer_manager->texture_effect = TextureEffect::NONE;
  }
  for (auto &[xpos, effect_frame_queue] : effect_frame_queue_map) {
    if (!effect_frame_queue.empty()) {
      renderer_manager->addRect(effect_frame_queue.front().first,
                                effect_frame_queue.front().second,
                                QColor(255, 182, 193, 240), 0, true);
      // if (effect_frame_queue.front().is_last_frame) {
      //   auto note =
      //       std::static_pointer_cast<Note>(effect_frame_queue.front().obj_ref);
      //   if (note->note_type == NoteType::HOLD) {
      //     std::static_pointer_cast<Hold>(note)->effect_play_over = true;
      //   }
      // }
      // 弹出队首
      if (!effect_frame_queue.empty()) effect_frame_queue.pop();
    }
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
  editor->preference_bpm = nullptr;
  editor->canvas_pasued = true;

  if (map) {
    editor->map_type = map->maptype;
    auto s = QDir(map->audio_file_abs_path);
    // 更新谱面长度(如果音乐比谱面长)
    auto map_audio_length =
        BackgroundAudio::get_audio_length(s.canonicalPath().toStdString());
    // 更新编辑器的信息
    editor->update_size(size());

    // XINFO("maplength:" + std::to_string(map->map_length));
    // XINFO("audiolength:" + std::to_string(map_audio_length));
    if (map->map_length < map_audio_length) map->map_length = map_audio_length;
    // 设置谱面时间
    editor->current_visual_time_stamp =
        map->project_reference->map_canvasposes.at(map);

    // 加载背景图纹理
    auto ppath = map->bg_path.root_path();
    add_texture(ppath, map->bg_path);
  } else {
    editor->current_visual_time_stamp = 0;
  }

  emit pause_signal(editor->canvas_pasued);
  emit current_time_stamp_changed(editor->current_visual_time_stamp);
}
