#include "MapWorkspaceCanvas.h"

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qdir.h>
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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../log/colorful-log.h"
#include "../../mmm/MapWorkProject.h"
#include "../../mmm/hitobject/HitObject.h"
#include "../../mmm/hitobject/Note/Note.h"
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
  // 初始化特效线程
  effect_thread = std::make_unique<EffectThread>(editor);
  // 连接信号
  connect(this, &MapWorkspaceCanvas::pause_signal, effect_thread.get(),
          &EffectThread::on_canvas_pause);
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

  // 初始化时间信息生成器
  timegenerator = std::make_shared<TimeInfoGenerator>(editor);
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
  // 同步一下时间
  effect_thread->sync_music_time(editor->current_time_stamp -
                                 (1.0 - editor->playspeed) *
                                     editor->audio_buffer_offset);
}

// 同步音频播放时间
void MapWorkspaceCanvas::on_music_pos_sync(double time) {
  editor->current_time_stamp =
      time - (1.0 - editor->playspeed) * editor->audio_buffer_offset;
  editor->current_visual_time_stamp =
      editor->current_time_stamp + editor->static_time_offset;
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
    working_map->project_reference->map_canvasposes.at(working_map) =
        editor->current_time_stamp;
    emit current_time_stamp_changed(editor->current_time_stamp);
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
      // 设置状态和位置快照
      editor->mouse_left_pressed = true;
      editor->mouse_left_press_pos = event->pos();
      // 更新选中信息
      editor->update_selections(event->modifiers() & Qt::ControlModifier);
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
    // 正在拖动
    if (editor->edit_mode == MouseEditMode::SELECT) {
      // 选择模式-更新选中信息
      // 正悬浮在物件上
      if (editor->hover_hitobject_info) {
        // 调整物件时间戳等属性
      } else {
        // 未悬浮在任何物件上-更新选中框定位点
        editor->update_selection_area(event->pos(),
                                      event->modifiers() & Qt::ControlModifier);
      }
    }

    if (editor->edit_mode == MouseEditMode::PLACE_NOTE) {
      // 放置物件模式-拖动中--更新正在放置的物件的时间戳
    }

    if (editor->edit_mode == MouseEditMode::PLACE_LONGNOTE) {
      // 放置长键模式-拖动中--更新正在放置的长条的持续时间
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

  auto dy = event->angleDelta().y();
  // 编辑区
  if (modifiers & Qt::ControlModifier) {
    // 在编辑区-按下controll滚动
    // 修改时间线缩放
    editor->scroll_update_timelinezoom(dy);
    return;
  }
  if (modifiers & Qt::AltModifier) {
    // 在编辑区-按下alt滚动
    // 获取鼠标位置的拍--修改此拍分拍策略/改为自定义
    return;
  }

  editor->update_timepos(dy, modifiers & Qt::ShiftModifier);
}

// 键盘按下事件
void MapWorkspaceCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyPressEvent(event);

  // 捕获按键
  auto keycode = event->key();

  switch (keycode) {
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4: {
      // 快速切换编辑模式
      editor->edit_mode = static_cast<MouseEditMode>(keycode - Qt::Key_0);
      emit switch_edit_mode(editor->edit_mode);
      break;
    }
    case Qt::Key_Space: {
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
      // AudioEnginPlayCallback::count = 0;
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

// 调整尺寸事件
void MapWorkspaceCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  GLCanvas::resizeEvent(event);
  editor->update_size(size());
  editor->update_areas();
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

  // 预览区域判定线
  // renderer_manager->addLine(
  //     QPointF(current_size.width() * (1 - editor->preview_width_scale),
  //             current_size.height() / 2.0),
  //     QPointF(current_size.width(), current_size.height() / 2.0), 6, nullptr,
  //     QColor(0, 255, 255, 235), false);
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
}

// 绘制信息区
void MapWorkspaceCanvas::draw_infoarea() { timegenerator->generate(); }

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
  switch (etype) {
    case EffectType::NORMAL: {
      std::shared_ptr<TextureInstace> &effect_frame_texture =
          texture_full_map[skin.nomal_hit_effect_dir + "/1.png"];
      for (int i = 1; i <= frame_count; ++i) {
        auto w =
            effect_frame_texture->width * (editor->object_size_scale * 0.75);
        auto h =
            effect_frame_texture->height * (editor->object_size_scale * 0.75);
        // TODO-xiang-:不知名入队bug
        auto frame_texname =
            skin.nomal_hit_effect_dir + "/" +
            std::to_string(i % skin.nomal_hit_effect_frame_count + 1) + ".png";
        auto frame =
            std::make_pair(QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
                           texture_full_map[frame_texname]);
        effect_frame_queue_map[xpos].push(frame);
      }
      break;
    }
    case EffectType::SLIDEARROW: {
      std::shared_ptr<TextureInstace> &effect_frame_texture =
          texture_full_map[skin.slide_hit_effect_dir + "/1.png"];
      for (int i = 1; i <= frame_count; ++i) {
        auto w =
            effect_frame_texture->width * (editor->object_size_scale * 0.75);
        auto h =
            effect_frame_texture->height * (editor->object_size_scale * 0.75);
        auto frame_texname =
            skin.slide_hit_effect_dir + "/" +
            std::to_string(i % skin.slide_hit_effect_frame_count + 1) + ".png";
        auto frame =
            std::make_pair(QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
                           texture_full_map[frame_texname]);
        effect_frame_queue_map[xpos].push(frame);
      }
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
  // 计算图形
  for (const auto &obj : editor->buffer_objects) {
    if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX) continue;
    auto note = std::static_pointer_cast<Note>(obj);
    if (!note) continue;
    if (drawed_objects.find(note) == drawed_objects.end())
      drawed_objects.insert({note, true});
    else
      continue;
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
    if (editor->show_object_after_judgeline) {
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
    } else {
      if (!editor->canvas_pasued && shape.objref &&
          shape.objref->timestamp <= editor->current_time_stamp) {
      } else {
        renderer_manager->addRect(QRectF(shape.x, shape.y, shape.w, shape.h),
                                  shape.tex, QColor(0, 0, 0, 255), 0, true);
      }
    }
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

    // 绘制物件
    draw_hitobject();
    renderer_manager->texture_effect = TextureEffect::NONE;
  }

  // 绘制特效
  for (auto &[xpos, effect_frame_queue] : effect_frame_queue_map) {
    if (!effect_frame_queue.empty()) {
      renderer_manager->addRect(effect_frame_queue.front().first,
                                effect_frame_queue.front().second,
                                QColor(255, 182, 193, 240), 0, true);
      // 弹出队首
      if (!effect_frame_queue.empty()) effect_frame_queue.pop();
    }
  }

  // 绘制选中区域
  draw_select_bound();
  // 绘制预览区域
  draw_preview_content();

  // 绘制判定线
  draw_judgeline();
  // 绘制信息区域
  draw_infoarea();

  // 绘制顶部栏
  draw_top_bar();
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> map) {
  // 此处确保了未打开项目是无法switchmap的(没有选项)
  // 先断开当前信号
  if (working_map)
    disconnect(working_map->audio_pos_callback.get(),
               &AudioEnginPlayCallback::music_play_callback, this,
               &MapWorkspaceCanvas::on_music_pos_sync);
  working_map = map;
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

    BackgroundAudio::play_audio(working_map->project_reference->devicename,
                                s.canonicalPath().toStdString());
    BackgroundAudio::pause_audio(working_map->project_reference->devicename,
                                 s.canonicalPath().toStdString());

    // 同步谱面时间
    editor->current_time_stamp =
        map->project_reference->map_canvasposes.at(map);
    editor->current_visual_time_stamp =
        editor->current_time_stamp + editor->static_time_offset;
    effect_thread->sync_music_time(editor->current_time_stamp);

    // 同步特效线程的map
    effect_thread->disconnect_current_callback();
    effect_thread->update_map();

    // 同步槽新信号
    connect(working_map->audio_pos_callback.get(),
            &AudioEnginPlayCallback::music_play_callback, this,
            &MapWorkspaceCanvas::on_music_pos_sync);

    // 同步音频的时间
    if (working_map->project_reference->devicename != "unknown output device") {
      BackgroundAudio::set_audio_pos(working_map->project_reference->devicename,
                                     s.canonicalPath().toStdString(),
                                     editor->current_time_stamp);
    }

    // 加载背景图纹理
    auto ppath = map->bg_path.root_path();
    add_texture(ppath, map->bg_path);
  } else {
    editor->current_visual_time_stamp = 0;
  }

  emit pause_signal(editor->canvas_pasued);
  emit current_time_stamp_changed(editor->current_visual_time_stamp);
}
