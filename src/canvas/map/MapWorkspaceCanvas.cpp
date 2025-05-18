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
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "../../log/colorful-log.h"
#include "../../mmm/MapWorkProject.h"
#include "../../mmm/hitobject/HitObject.h"
#include "../../mmm/hitobject/Note/Note.h"
#include "../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../audio/BackgroundAudio.h"
#include "MapWorkspaceSkin.h"
#include "editor/MapEditor.h"
#include "generator/OrbitGenerator.hpp"
#include "generator/general/HoldGenerator.h"
#include "generator/general/NoteGenerator.h"
#include "generator/general/SlideGenerator.h"
#include "mainwindow.h"

MapWorkspaceCanvas::MapWorkspaceCanvas(QWidget *parent)
    : GLCanvas(parent), skin(this), canvas_tpool(4) {
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

    // 初始化预览生成器
    previewgenerator = std::make_shared<PreviewGenerator>(editor);

    // 初始化轨道生成器
    orbitgenerator = std::make_shared<OrbitGenerator>(editor);
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
    editor->cstatus.canvas_pasued = paused;
    emit pause_signal(paused);
}

// 时间控制器播放速度变化
void MapWorkspaceCanvas::on_timecontroller_speed_changed(double speed) {
    editor->cstatus.playspeed = speed;
    // 同步一下时间
    effect_thread->sync_music_time(editor->cstatus.current_time_stamp -
                                   (1.0 - editor->cstatus.playspeed) *
                                       BackgroundAudio::audio_buffer_offset);
}

// 同步音频播放时间
void MapWorkspaceCanvas::on_music_pos_sync(double time) {
    editor->cstatus.current_time_stamp =
        time - (1.0 - editor->cstatus.playspeed) *
                   BackgroundAudio::audio_buffer_offset;
    editor->cstatus.current_visual_time_stamp =
        editor->cstatus.current_time_stamp + editor->cstatus.static_time_offset;
}

// 时间编辑器设置精确时间
void MapWorkspaceCanvas::on_timeedit_setpos(double time) {
    editor->cstatus.current_time_stamp = time;
    editor->cstatus.current_visual_time_stamp =
        editor->cstatus.current_time_stamp + editor->cstatus.static_time_offset;
}

// qt事件
void MapWorkspaceCanvas::paintEvent(QPaintEvent *event) {
    GLCanvas::paintEvent(event);

    static long long lasttime = 0;
    auto time =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    auto atime = time - lasttime;
    actual_update_time = double(atime) / 1000.0;
    if (lasttime > 0 &&
        actual_update_time - des_update_time > des_update_time) {
        auto log = tr("qt update substantially delayed:%1ms")
                       .arg(QString::number(actual_update_time, 'f', 2));
        XWARN(log.toStdString());
    }
    lasttime = time;

    if (!editor->cstatus.canvas_pasued) {
        // 未暂停,更新当前时间戳
        editor->cstatus.current_time_stamp =
            editor->cstatus.current_time_stamp +
            actual_update_time * editor->cstatus.playspeed;

        if (editor->cstatus.current_time_stamp < 0) {
            editor->cstatus.current_time_stamp = 0;
            editor->cstatus.canvas_pasued = true;
            emit pause_signal(editor->cstatus.canvas_pasued);
        }
        if (working_map &&
            editor->cstatus.current_time_stamp > working_map->map_length) {
            editor->cstatus.current_time_stamp = working_map->map_length;
            editor->cstatus.canvas_pasued = true;
            emit pause_signal(editor->cstatus.canvas_pasued);
        }
        editor->cstatus.current_visual_time_stamp =
            editor->cstatus.current_time_stamp +
            editor->cstatus.static_time_offset;
        working_map->project_reference->map_canvasposes.at(working_map) =
            editor->cstatus.current_time_stamp;
        emit current_time_stamp_changed(editor->cstatus.current_time_stamp);
    }
}

// 更新fps显示
void MapWorkspaceCanvas::updateFpsDisplay(int fps) {
    QString title_suffix =
        QString(
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

    switch (event->button()) {
        case Qt::MouseButton::LeftButton: {
            editor->cstatus.mouse_left_pressed = true;
            editor->cstatus.mouse_left_press_pos = event->pos();
            if (!editor->cstatus.mouse_right_pressed) {
                editor->mouse_pressed(event);
            }
            break;
        }
        case Qt::MouseButton::RightButton: {
            editor->cstatus.mouse_right_pressed = true;
            if (!editor->cstatus.mouse_left_pressed) {
                editor->mouse_pressed(event);
            }
            break;
        }
    }

    // qDebug() << event->button();
}

// 鼠标释放事件
void MapWorkspaceCanvas::mouseReleaseEvent(QMouseEvent *event) {
    // 传递事件
    GLCanvas::mouseReleaseEvent(event);

    switch (event->button()) {
        case Qt::MouseButton::LeftButton: {
            if (!editor->cstatus.mouse_right_pressed) {
                editor->mouse_released(event);
            }
            editor->cstatus.mouse_left_pressed = false;
            break;
        }
        case Qt::MouseButton::RightButton: {
            if (!editor->cstatus.mouse_left_pressed) {
                editor->mouse_released(event);
            }
            editor->cstatus.mouse_right_pressed = false;
            break;
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
    // 传递事件给map编辑器
    editor->mouse_moved(event);
}

// 鼠标滚动事件
void MapWorkspaceCanvas::wheelEvent(QWheelEvent *event) {
    // 传递事件
    GLCanvas::wheelEvent(event);
    if (!working_map) return;
    editor->mouse_scrolled(event);
}

// 键盘按下事件
void MapWorkspaceCanvas::keyPressEvent(QKeyEvent *event) {
    // 传递事件
    GLCanvas::keyPressEvent(event);

    // 捕获按键
    auto keycode = event->key();
    auto modifiers = event->modifiers();

    switch (keycode) {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5: {
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
            editor->cstatus.canvas_pasued = !editor->cstatus.canvas_pasued;
            emit pause_signal(editor->cstatus.canvas_pasued);
            // AudioEnginPlayCallback::count = 0;
            break;
        }
        case Qt::Key_C: {
            if (modifiers & Qt::ControlModifier) {
                // 复制
                editor->copy();
            }
            break;
        }
        case Qt::Key_V: {
            if (modifiers & Qt::ControlModifier) {
                // 粘贴
                editor->paste();
            }
            break;
        }
        case Qt::Key_Z: {
            if (modifiers & Qt::ControlModifier) {
                // 撤回
                editor->undo();
                XWARN("已撤回");
            }
            break;
        }
        case Qt::Key_Y: {
            if (modifiers & Qt::ControlModifier) {
                // 重做
                editor->redo();
                XWARN("已重做");
            }
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
    std::lock_guard<std::mutex> lock(skin_mtx);
    editor->update_size(size());
}

// 绘制背景
void MapWorkspaceCanvas::draw_background() {
    if (!working_map) return;
    auto des = QRectF(0, 0, width(), height());

    // 绘制背景图
    renderer_manager->texture_fillmode = TextureFillMode::SCALLING_AND_TILE;
    auto bg_str = working_map->bg_path.generic_string();
    std::replace(bg_str.begin(), bg_str.end(), '\\', '/');
    auto &t = texture_full_map[bg_str];
    renderer_manager->addRect(des, t, QColor(0, 0, 0, 255), 0, false);
    // 绘制背景遮罩
    if (editor->cstatus.background_darken_ratio != 0.0) {
        renderer_manager->addRect(
            des, nullptr,
            QColor(0, 0, 0, 255 * editor->cstatus.background_darken_ratio), 0,
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
    renderer_manager->addRoundRect(top_bar_out, nullptr,
                                   QColor(33, 33, 33, 230), 0, 1.3, false);
    renderer_manager->addLine(QPointF(0, 0), QPointF(current_size.width(), 0),
                              2.0f, nullptr, QColor(255, 255, 255, 240), false);
}

// 绘制选中框
void MapWorkspaceCanvas::draw_select_bound() {
    if (editor->ebuffer.select_bound_locate_points) {
        auto &border_left_texture =
            skin.get_selected_border_texture(SelectBorderDirection::LEFT);
        auto &border_right_texture =
            skin.get_selected_border_texture(SelectBorderDirection::RIGHT);
        auto &border_top_texture =
            skin.get_selected_border_texture(SelectBorderDirection::TOP);
        auto &border_bottom_texture =
            skin.get_selected_border_texture(SelectBorderDirection::BOTTOM);

        auto p1 = editor->ebuffer.select_bound_locate_points->first;
        auto p2 =
            QPointF(editor->ebuffer.select_bound_locate_points->first.x(),
                    editor->ebuffer.select_bound_locate_points->second.y());
        auto p3 = editor->ebuffer.select_bound_locate_points->second;
        auto p4 =
            QPointF(editor->ebuffer.select_bound_locate_points->second.x(),
                    editor->ebuffer.select_bound_locate_points->first.y());

        // 左矩形p1-p2
        auto leftrect = QRectF(
            p1.x() - editor->csettings.select_border_width / 2.0,
            p1.y() < p2.y()
                ? p1.y() - editor->csettings.select_border_width / 2.0
                : p2.y() - editor->csettings.select_border_width / 2.0,
            editor->csettings.select_border_width,
            std::abs(p2.y() - p1.y()) + editor->csettings.select_border_width);

        // 右矩形p3-p4
        auto rightrect = QRectF(
            p4.x() - editor->csettings.select_border_width / 2.0,
            p4.y() < p3.y()
                ? p4.y() - editor->csettings.select_border_width / 2.0
                : p3.y() - editor->csettings.select_border_width / 2.0,
            editor->csettings.select_border_width,
            std::abs(p3.y() - p4.y()) + editor->csettings.select_border_width);

        // 上矩形p1-p4
        auto toprect = QRectF(
            p1.x() < p4.x()
                ? p1.x() - editor->csettings.select_border_width / 2.0
                : p4.x() - editor->csettings.select_border_width / 2.0,
            p1.y() - editor->csettings.select_border_width / 2.0,
            std::abs(p4.x() - p1.x()) + editor->csettings.select_border_width,
            editor->csettings.select_border_width);

        // 下矩形p2-p3
        auto bottomrect = QRectF(
            p2.x() < p3.x()
                ? p2.x() - editor->csettings.select_border_width / 2.0
                : p3.x() - editor->csettings.select_border_width / 2.0,
            p2.y() - editor->csettings.select_border_width / 2.0,
            std::abs(p3.x() - p2.x()) + editor->csettings.select_border_width,
            editor->csettings.select_border_width);

        renderer_manager->addRect(leftrect, border_left_texture,
                                  QColor(0, 0, 0, 255), 0, true);
        renderer_manager->addRect(rightrect, border_right_texture,
                                  QColor(0, 0, 0, 255), 0, true);
        renderer_manager->addRect(toprect, border_top_texture,
                                  QColor(0, 0, 0, 255), 0, true);
        renderer_manager->addRect(bottomrect, border_bottom_texture,
                                  QColor(0, 0, 0, 255), 0, true);
    }
}

// 绘制预览
void MapWorkspaceCanvas::draw_preview_content() {
    previewgenerator->generate();
}

// 绘制轨道底板
void MapWorkspaceCanvas::draw_orbits() {}

// 绘制判定线
void MapWorkspaceCanvas::draw_judgeline() {
    auto current_size = size();

    // 主区域判定线
    renderer_manager->addLine(
        QPointF(0, editor->ebuffer.judgeline_visual_position),
        QPointF(
            current_size.width() * (1 - editor->csettings.preview_width_scale),
            editor->ebuffer.judgeline_visual_position),
        8, nullptr, QColor(0, 255, 255, 235), false);
}

// 绘制信息区
void MapWorkspaceCanvas::draw_infoarea() { timegenerator->generate(); }

// 绘制拍
void MapWorkspaceCanvas::draw_beats() {
    if (!working_map) return;
    // 生成图形数据
    beatgenerator->generate();
    decltype(texture_full_map.begin()) s;

    // 先绘制所有节拍线
    while (!BeatGenerator::line_queue.empty()) {
        auto &line = BeatGenerator::line_queue.front();
        renderer_manager->addLine(QPointF(line.x1, line.y1),
                                  QPointF(line.x2, line.y2), line.line_width,
                                  nullptr,
                                  QColor(line.r, line.g, line.b, line.a), true);

        BeatGenerator::line_queue.pop();
    }

    // 绘制分拍背景
    while (!BeatGenerator::divbg_queue.empty()) {
        auto &divbg_bound = BeatGenerator::divbg_queue.front();
        renderer_manager->addRoundRect(divbg_bound, nullptr,
                                       QColor(64, 64, 64, 233), 0, 1.1, true);
        BeatGenerator::divbg_queue.pop();
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
                    effect_frame_texture->width *
                    (editor->ebuffer.object_size_scale * 0.75 *
                     working_map->project_reference->config.object_width_ratio);
                auto h = effect_frame_texture->height *
                         (editor->ebuffer.object_size_scale * 0.75 *
                          working_map->project_reference->config
                              .object_height_ratio);
                auto frame_texname =
                    skin.nomal_hit_effect_dir + "/" +
                    std::to_string(i % skin.nomal_hit_effect_frame_count + 1) +
                    ".png";
                auto frame =
                    std::make_pair(QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
                                   texture_full_map[frame_texname]);
                std::lock_guard<std::mutex> lock(
                    effect_frame_queue_map[xpos].mtx);
                effect_frame_queue_map[xpos].queue.push(frame);
            }
            break;
        }
        case EffectType::SLIDEARROW: {
            std::shared_ptr<TextureInstace> &effect_frame_texture =
                texture_full_map[skin.slide_hit_effect_dir + "/1.png"];
            for (int i = 1; i <= frame_count; ++i) {
                auto w =
                    effect_frame_texture->width *
                    (editor->ebuffer.object_size_scale * 0.75 *
                     working_map->project_reference->config.object_width_ratio);
                auto h = effect_frame_texture->height *
                         (editor->ebuffer.object_size_scale * 0.75 *
                          working_map->project_reference->config
                              .object_height_ratio);
                auto frame_texname =
                    skin.slide_hit_effect_dir + "/" +
                    std::to_string(i % skin.slide_hit_effect_frame_count + 1) +
                    ".png";
                auto frame =
                    std::make_pair(QRectF(xpos - w / 2.0, ypos - h / 2.0, w, h),
                                   texture_full_map[frame_texname]);
                std::lock_guard<std::mutex> lock(
                    effect_frame_queue_map[xpos].mtx);
                effect_frame_queue_map[xpos].queue.push(frame);
            }
            break;
        }
    }
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> &objects,
    HitObjectEffect e) {
    // 防止重复绘制
    std::unordered_map<std::shared_ptr<HitObject>, bool> drawed_objects;

    // 渲染物件
    // 计算图形
    for (const auto &obj : objects) {
        if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX)
            continue;
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

    // 切换纹理填充效果
    TextureEffect effect;
    switch (e) {
        case HitObjectEffect::NORMAL: {
            effect = TextureEffect::NONE;
            break;
        }
        case HitObjectEffect::SHADOW: {
            effect = TextureEffect::HALF_TRANSPARENT;
            break;
        }
    }
    renderer_manager->texture_effect = effect;

    // 按计算层级渲染图形
    while (!ObjectGenerator::shape_queue.empty()) {
        auto &shape = ObjectGenerator::shape_queue.front();
        if (e == HitObjectEffect::NORMAL) {
            if (editor->csettings.show_object_after_judgeline) {
                if (shape.is_over_current_time) {
                    renderer_manager->texture_effect =
                        TextureEffect::HALF_TRANSPARENT;
                }
                if (!editor->cstatus.canvas_pasued && shape.objref &&
                    shape.objref->timestamp <=
                        editor->cstatus.current_time_stamp) {
                    // 播放中且过了判定线时间使用半透明效果
                    renderer_manager->texture_effect =
                        TextureEffect::HALF_TRANSPARENT;
                }

                renderer_manager->addRect(
                    QRectF(shape.x, shape.y, shape.w, shape.h), shape.tex,
                    QColor(0, 0, 0, 255), 0, true);
            } else {
                if (!editor->cstatus.canvas_pasued && shape.objref &&
                    shape.objref->timestamp <=
                        editor->cstatus.current_time_stamp) {
                } else {
                    renderer_manager->addRect(
                        QRectF(shape.x, shape.y, shape.w, shape.h), shape.tex,
                        QColor(0, 0, 0, 255), 0, true);
                }
            }
            renderer_manager->texture_effect = TextureEffect::NONE;
        } else {
            renderer_manager->addRect(
                QRectF(shape.x, shape.y, shape.w, shape.h), shape.tex,
                QColor(0, 0, 0, 255), 0, true);
        }
        ObjectGenerator::shape_queue.pop();
    }
}

void MapWorkspaceCanvas::remove_objects(std::shared_ptr<HitObject> o) {
    auto it = editor->ebuffer.buffer_objects.lower_bound(o);
    // 前移到上一时间戳
    while (it != editor->ebuffer.buffer_objects.begin() &&
           o->timestamp - it->get()->timestamp < 10)
        --it;
    // 需要收集所有要删除的迭代器，因为删除会影响迭代器
    std::vector<decltype(it)> to_erase;

    while (it != editor->ebuffer.buffer_objects.end() &&
           (it->get()->timestamp - o->timestamp) < 5) {
        if (it->get()->equals(o)) {
            to_erase.push_back(it);
        }
        ++it;
    }

    // 从后往前删除，避免迭代器失效问题
    for (auto rit = to_erase.rbegin(); rit != to_erase.rend(); ++rit) {
        // XWARN(QString("隐藏编辑中的源物件:[%1]")
        //       .arg((*rit)->get()->timestamp)
        //       .toStdString());
        editor->ebuffer.buffer_objects.erase(*rit);
    }
}

// 渲染实际图形
void MapWorkspaceCanvas::push_shape() {
    // 绘制背景
    draw_background();

    if (working_map) {
        // 清除hover信息
        // 生成区域信息
        areagenerator->generate();
        if (editor->csettings.show_timeline) {
            draw_beats();
        }

        // 更新物件列表
        // 清除物件缓存
        editor->ebuffer.buffer_objects.clear();
        working_map->query_object_in_range(
            editor->ebuffer.buffer_objects,
            int32_t(editor->ebuffer.current_time_area_start),
            int32_t(editor->ebuffer.current_time_area_end), true);

        for (const auto &[type, obj_editor] : editor->obj_editors) {
            // 不显示编辑中的物件-移除
            for (const auto &o : obj_editor->editing_src_objects) {
                auto comp = std::dynamic_pointer_cast<ComplexNote>(o);
                if (comp) {
                    for (const auto &child_note : comp->child_notes) {
                        remove_objects(child_note);
                    }
                }
                remove_objects(o);
            }
        }

        // 绘制map原本的物件
        draw_hitobject(editor->ebuffer.buffer_objects, HitObjectEffect::NORMAL);
        // 更新hover信息
        if (!editor->cstatus.is_hover_note) {
            // 未悬浮在任何一个物件或物件身体上
            editor->ebuffer.hover_object_info = nullptr;
        } else {
            editor->cstatus.is_hover_note = false;
        }

        // 绘制虚影物件-编辑缓存
        std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
            editbuffers;
        for (const auto &[type, obj_editor] : editor->obj_editors) {
            if (obj_editor->editing_temp_objects.empty()) continue;
            for (const auto &temp_shadow_obj :
                 obj_editor->editing_temp_objects) {
                auto comp =
                    std::dynamic_pointer_cast<ComplexNote>(temp_shadow_obj);
                if (comp) {
                    for (const auto &child_note : comp->child_notes) {
                        editbuffers.insert(child_note);
                    }
                }
                editbuffers.insert(temp_shadow_obj);
            }
        }
        draw_hitobject(editbuffers, HitObjectEffect::SHADOW);

        renderer_manager->texture_effect = TextureEffect::NONE;
        // 绘制预览区域
        draw_preview_content();

        // 绘制选中区域
        draw_select_bound();

        // 绘制判定线
        draw_judgeline();
        // 绘制信息区域
        draw_infoarea();
        // 绘制顶部栏
        draw_top_bar();
        // 绘制特效
        for (auto &[xpos, effect_frame_queue] : effect_frame_queue_map) {
            std::lock_guard<std::mutex> lock(effect_frame_queue.mtx);
            if (!effect_frame_queue.queue.empty()) {
                renderer_manager->addRect(
                    effect_frame_queue.queue.front().first,
                    effect_frame_queue.queue.front().second,
                    QColor(255, 182, 193, 240), 0, true);
                // 弹出队首
                effect_frame_queue.queue.pop();
            }
        }
    }
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
    editor->cstatus.canvas_pasued = true;

    // 清理图形缓存
    editor->ebuffer.current_beats.clear();
    effect_frame_queue_map.clear();

    if (map) {
        editor->cstatus.map_type = map->maptype;
        editor->edit_method = map->project_reference->config.edit_method;
        auto s = QDir(map->audio_file_abs_path);
        // 更新谱面长度(如果音乐比谱面长)
        auto map_audio_length =
            BackgroundAudio::get_audio_length(s.canonicalPath().toStdString());
        // 更新编辑器的信息
        editor->update_size(size());

        // XINFO("maplength:" + std::to_string(map->map_length));
        // XINFO("audiolength:" + std::to_string(map_audio_length));
        if (map->map_length < map_audio_length)
            map->map_length = map_audio_length;

        BackgroundAudio::play_audio(working_map->project_reference->devicename,
                                    s.canonicalPath().toStdString());
        BackgroundAudio::pause_audio(working_map->project_reference->devicename,
                                     s.canonicalPath().toStdString());

        // 同步谱面时间
        editor->cstatus.current_time_stamp =
            map->project_reference->map_canvasposes.at(map);
        editor->cstatus.current_visual_time_stamp =
            editor->cstatus.current_time_stamp +
            editor->cstatus.static_time_offset;
        effect_thread->sync_music_time(editor->cstatus.current_time_stamp);

        // 同步特效线程的map
        effect_thread->disconnect_current_callback();
        effect_thread->update_map();

        // 同步槽新信号
        connect(working_map->audio_pos_callback.get(),
                &AudioEnginPlayCallback::music_play_callback, this,
                &MapWorkspaceCanvas::on_music_pos_sync);

        // 同步音频的时间
        if (working_map->project_reference->devicename !=
            "unknown output device") {
            BackgroundAudio::set_audio_pos(
                working_map->project_reference->devicename,
                s.canonicalPath().toStdString(),
                editor->cstatus.current_time_stamp);
        }

        // 加载背景图纹理
        auto ppath = map->bg_path.root_path();
        add_texture(ppath, map->bg_path);
    } else {
        editor->cstatus.current_visual_time_stamp = 0;
    }

    emit pause_signal(editor->cstatus.canvas_pasued);
    emit current_time_stamp_changed(editor->cstatus.current_visual_time_stamp);
}
