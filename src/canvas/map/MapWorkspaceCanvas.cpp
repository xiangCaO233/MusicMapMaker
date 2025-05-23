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
#include "../../util/mutil.h"
#include "../audio/BackgroundAudio.h"
#include "MapWorkspaceSkin.h"
#include "editor/MapEditor.h"
#include "generator/OrbitGenerator.h"
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

    // 初始化判定线生成器
    judgelinegenerator = std::make_shared<JudgelineGenerator>(editor);

    average_update_time = des_update_time;
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
    // 更新平均帧时间
    average_update_time = (average_update_time + actual_update_time) / 2.0;
    if (lasttime > 0 &&
        actual_update_time - des_update_time > des_update_time) {
        auto log = tr("qt update substantially delayed:%1ms")
                       .arg(QString::number(actual_update_time, 'f', 2));
        // XWARN(log.toStdString());
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
            editor->cstatus.mouse_left_press_pos = event->pos();
            if (!editor->cstatus.mouse_right_pressed) {
                editor->cstatus.mouse_left_pressed = true;
                editor->mouse_pressed(event);
            }
            break;
        }
        case Qt::MouseButton::RightButton: {
            if (!editor->cstatus.mouse_left_pressed) {
                editor->cstatus.mouse_right_pressed = true;
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
        case Qt::Key_S: {
            if (modifiers & Qt::ControlModifier) {
                // 直接保存
                if (working_map) {
                    auto dirpath = working_map->map_file_path.parent_path();

                    if (working_map->project_reference->config
                            .alway_save_asmmm) {
                        // 保存为mmm
                        auto def_filename = mutil::sanitizeFilename(
                            working_map->title_unicode + "-" +
                            std::to_string(working_map->orbits) + "k-" +
                            working_map->version + ".mmm");
                        auto map_path = dirpath / def_filename;
                        working_map->write_to_file(
                            map_path.generic_string().c_str());

                    } else {
                        // 覆盖原有的文件
                        working_map->write_to_file(
                            working_map->map_file_path.generic_string()
                                .c_str());
                    }
                    XINFO("已保存");
                } else {
                    XWARN("未打开谱面");
                }
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
void MapWorkspaceCanvas::draw_background(BufferWrapper *bufferwrapper) {
    if (!working_map) return;
    // 绘制背景图

    auto &params_list = bufferwrapper->bg_datas.emplace();
    auto &params = params_list.emplace_back();

    // 绘制矩形
    params.func_type = FunctionType::MRECT;

    // 矩形位置
    params.xpos = 0;
    params.ypos = 0;
    params.width = width();
    params.height = height();

    params.render_settings.texture_fillmode =
        TextureFillMode::SCALLING_AND_TILE;
    auto bg_str = working_map->bg_path.generic_string();
    std::replace(bg_str.begin(), bg_str.end(), '\\', '/');
    params.texture = texture_full_map[bg_str];
    params.is_volatile = false;

    // 绘制背景遮罩
    if (editor->cstatus.background_darken_ratio != 0.0) {
        auto &params = params_list.emplace_back();
        params.xpos = 0;
        params.ypos = 0;
        params.width = width();
        params.height = height();
        params.texture = nullptr;
        params.a = 255 * editor->cstatus.background_darken_ratio;
        params.is_volatile = false;
    }
}

// 绘制顶部栏
void MapWorkspaceCanvas::draw_top_bar(BufferWrapper *bufferwrapper) {
    auto current_size = size();
    auto &topbar_params_list = bufferwrapper->topbar_datas.emplace();

    QRectF top_bar_out(0.0f, current_size.height() / 12.0f * -0.3f,
                       current_size.width(), current_size.height() / 12.0f);
    QRectF top_bar_in(
        current_size.width() / 48.0f,
        current_size.height() / 12.0f * -0.3f + current_size.height() / 48.0f,
        current_size.width() - (current_size.width() / 48.0f),
        current_size.height() / 12.0f - (current_size.height() / 48.0f));

    auto &topbar_params1 = topbar_params_list.emplace_back();
    topbar_params1.func_type = FunctionType::MROUNDRECT;
    topbar_params1.xpos = top_bar_in.x();
    topbar_params1.ypos = top_bar_in.x();
    topbar_params1.width = top_bar_in.width();
    topbar_params1.height = top_bar_in.height();
    topbar_params1.radius = 1.3;
    topbar_params1.r = 30;
    topbar_params1.g = 40;
    topbar_params1.b = 50;
    topbar_params1.a = 230;
    topbar_params1.is_volatile = false;

    auto &topbar_params2 = topbar_params_list.emplace_back();
    topbar_params2.func_type = FunctionType::MROUNDRECT;
    topbar_params2.xpos = top_bar_out.x();
    topbar_params2.ypos = top_bar_out.x();
    topbar_params2.width = top_bar_out.width();
    topbar_params2.height = top_bar_out.height();
    topbar_params2.radius = 1.3;
    topbar_params2.r = 33;
    topbar_params2.g = 33;
    topbar_params2.b = 33;
    topbar_params2.a = 230;
    topbar_params2.is_volatile = false;

    auto &topbar_params3 = topbar_params_list.emplace_back();
    topbar_params3.func_type = FunctionType::MLINE;
    topbar_params3.x1 = 0;
    topbar_params3.y1 = 0;
    topbar_params3.x2 = current_size.width();
    topbar_params3.y2 = 0;
    topbar_params3.line_width = 2.0f;
    topbar_params3.r = 255;
    topbar_params3.g = 255;
    topbar_params3.b = 255;
    topbar_params3.a = 240;
    topbar_params3.is_volatile = false;
}

// 绘制选中框
void MapWorkspaceCanvas::draw_select_bound(BufferWrapper *bufferwrapper) {
    auto &selection_params_list = bufferwrapper->selection_datas.emplace();
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
        QRectF rect;
        std::shared_ptr<TextureInstace> texture;

#define NEW_PARAMS(selection_params, rect, tex)                                \
    selection_params.func_type = FunctionType::MRECT;                          \
    selection_params.render_settings.texture_fillmode = TextureFillMode::FILL; \
    selection_params.is_volatile = false;                                      \
    selection_params.texture = tex;                                            \
    selection_params.xpos = rect.x();                                          \
    selection_params.ypos = rect.y();                                          \
    selection_params.width = rect.width();                                     \
    selection_params.height = rect.height();
        auto &selection_params1 = selection_params_list.emplace_back();
        NEW_PARAMS(selection_params1, leftrect, border_left_texture);
        auto &selection_params2 = selection_params_list.emplace_back();
        NEW_PARAMS(selection_params2, rightrect, border_right_texture);
        auto &selection_params3 = selection_params_list.emplace_back();
        NEW_PARAMS(selection_params3, toprect, border_top_texture);
        auto &selection_params4 = selection_params_list.emplace_back();
        NEW_PARAMS(selection_params4, bottomrect, border_bottom_texture);
#undef NEW_PARAMS
    }
}

// 绘制预览
void MapWorkspaceCanvas::draw_preview_content(BufferWrapper *bufferwrapper) {
    previewgenerator->generate(bufferwrapper);
}

// 绘制轨道底板
void MapWorkspaceCanvas::draw_orbits(BufferWrapper *bufferwrapper) {
    orbitgenerator->generate(bufferwrapper);
}

// 绘制判定线
void MapWorkspaceCanvas::draw_judgeline(BufferWrapper *bufferwrapper) {
    judgelinegenerator->generate(bufferwrapper);
}

// 绘制信息区
void MapWorkspaceCanvas::draw_infoarea(BufferWrapper *bufferwrapper) {
    timegenerator->generate(bufferwrapper);
}

// 绘制拍
void MapWorkspaceCanvas::draw_beats(BufferWrapper *bufferwrapper) {
    if (!working_map) return;
    auto &line_params_list = bufferwrapper->beats_datas.emplace();

    // 生成图形数据
    beatgenerator->generate();
    decltype(texture_full_map.begin()) s;

    // 先绘制所有节拍线
    while (!BeatGenerator::line_queue.empty()) {
        auto &line = BeatGenerator::line_queue.front();
        auto &line_params = line_params_list.emplace_back();
        line_params.func_type = FunctionType::MLINE;
        line_params.x1 = line.x1;
        line_params.y1 = line.y1;
        line_params.x2 = line.x2;
        line_params.y2 = line.y2;
        line_params.line_width = line.line_width;
        line_params.texture = nullptr;
        line_params.r = line.r;
        line_params.g = line.g;
        line_params.b = line.b;
        line_params.a = line.a;
        line_params.is_volatile = true;

        BeatGenerator::line_queue.pop();
    }

    auto &divbg_params_list = bufferwrapper->beats_datas.emplace();
    // 绘制分拍信息的背景
    while (!BeatGenerator::divbg_queue.empty()) {
        auto &divbg_bound = BeatGenerator::divbg_queue.front();
        auto &divbg_params = divbg_params_list.emplace_back();
        divbg_params.func_type = FunctionType::MROUNDRECT;
        divbg_params.xpos = divbg_bound.x();
        divbg_params.ypos = divbg_bound.y();
        divbg_params.width = divbg_bound.width();
        divbg_params.height = divbg_bound.height();
        divbg_params.r = 64;
        divbg_params.g = 64;
        divbg_params.b = 64;
        divbg_params.a = 233;
        divbg_params.radius = 1.1;
        divbg_params.is_volatile = true;
        BeatGenerator::divbg_queue.pop();
    }

    auto &text_params_list = bufferwrapper->beats_datas.emplace();
    // 绘制所有时间字符串
    while (!BeatGenerator::text_queue.empty()) {
        auto &text = BeatGenerator::text_queue.front();
        auto &text_params = text_params_list.emplace_back();
        text_params.func_type = FunctionType::MTEXT;
        text_params.xpos = text.x;
        text_params.ypos = text.y;
        text_params.str = text.text;
        text_params.line_width = skin.timeinfo_font_size;
        text_params.font_family = skin.font_family.c_str();
        text_params.r = skin.timeinfo_font_color.red();
        text_params.g = skin.timeinfo_font_color.green();
        text_params.b = skin.timeinfo_font_color.blue();
        text_params.a = skin.timeinfo_font_color.alpha();
        text_params.is_volatile = false;

        BeatGenerator::text_queue.pop();
    }
}

// 播放特效
void MapWorkspaceCanvas::play_effect(double xpos, double ypos, double play_time,
                                     EffectType etype) {
    auto &queue = effect_frame_queue_map[xpos];
    std::lock_guard<std::mutex> lock(queue.mtx);
    queue.effect_type = etype;
    queue.time_left = play_time;
    queue.current_frame_pos = 1;
    queue.xpos = xpos;
    queue.ypos = ypos;
}

// 绘制物件
void MapWorkspaceCanvas::draw_hitobject(
    BufferWrapper *bufferwrapper,
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

    auto &obj_params_list = bufferwrapper->hitobjects_datas.emplace();
    // 按计算层级渲染图形
    while (!ObjectGenerator::shape_queue.empty()) {
        auto &shape = ObjectGenerator::shape_queue.front();
        if (e == HitObjectEffect::NORMAL) {
            if (editor->csettings.show_object_after_judgeline) {
                // 确定经过判定线后还显示
                auto &obj_params = obj_params_list.emplace_back();
                obj_params.func_type = FunctionType::MRECT;
                // 切换纹理绘制方式为填充
                obj_params.render_settings.texture_fillmode =
                    TextureFillMode::FILL;
                // 切换纹理绘制补齐方式为重采样
                obj_params.render_settings.texture_complementmode =
                    TextureComplementMode::REPEAT_TEXTURE;
                if (shape.is_over_current_time) {
                    obj_params.render_settings.texture_effect =
                        TextureEffect::HALF_TRANSPARENT;
                }
                if (!editor->cstatus.canvas_pasued && shape.objref &&
                    shape.objref->timestamp <=
                        editor->cstatus.current_time_stamp) {
                    // 播放中且过了判定线时间使用半透明效果
                    obj_params.render_settings.texture_effect =
                        TextureEffect::HALF_TRANSPARENT;
                }

                obj_params.xpos = shape.x;
                obj_params.ypos = shape.y;
                obj_params.width = shape.w;
                obj_params.height = shape.h;
                obj_params.texture = shape.tex;
                obj_params.is_volatile = true;

            } else {
                if (!editor->cstatus.canvas_pasued && shape.objref &&
                    shape.objref->timestamp <=
                        editor->cstatus.current_time_stamp) {
                } else {
                    auto &obj_params = obj_params_list.emplace_back();
                    obj_params.func_type = FunctionType::MRECT;
                    // 切换纹理绘制方式为填充
                    obj_params.render_settings.texture_fillmode =
                        TextureFillMode::FILL;
                    // 切换纹理绘制补齐方式为重采样
                    obj_params.render_settings.texture_complementmode =
                        TextureComplementMode::REPEAT_TEXTURE;
                    obj_params.xpos = shape.x;
                    obj_params.ypos = shape.y;
                    obj_params.width = shape.w;
                    obj_params.height = shape.h;
                    obj_params.texture = shape.tex;
                    obj_params.is_volatile = true;
                }
            }
        } else {
            auto &obj_params = obj_params_list.emplace_back();
            obj_params.func_type = FunctionType::MRECT;
            // 切换纹理绘制方式为填充
            obj_params.render_settings.texture_fillmode = TextureFillMode::FILL;
            // 切换纹理绘制补齐方式为重采样
            obj_params.render_settings.texture_complementmode =
                TextureComplementMode::REPEAT_TEXTURE;
            obj_params.xpos = shape.x;
            obj_params.ypos = shape.y;
            obj_params.width = shape.w;
            obj_params.height = shape.h;
            obj_params.texture = shape.tex;
            obj_params.is_volatile = true;
        }
        ObjectGenerator::shape_queue.pop();
    }
}

void MapWorkspaceCanvas::draw_effect_frame(BufferWrapper *bufferwrapper) {
    auto &effect_params_list = bufferwrapper->effects_datas.emplace();
    // 绘制特效
    for (auto &[xpos, effect_frame_queue] : effect_frame_queue_map) {
        std::lock_guard<std::mutex> lock(effect_frame_queue_map[xpos].mtx);
        if (effect_frame_queue.time_left > 0) {
            auto &effect_params = effect_params_list.emplace_back();
            std::string texdir;
            switch (effect_frame_queue.effect_type) {
                case EffectType::NORMAL: {
                    if (effect_frame_queue.current_frame_pos >
                        skin.nomal_hit_effect_frame_count) {
                        effect_frame_queue.current_frame_pos = 1;
                    }
                    texdir = skin.nomal_hit_effect_dir;
                    break;
                }
                case EffectType::SLIDEARROW: {
                    if (effect_frame_queue.current_frame_pos >
                        skin.slide_hit_effect_frame_count) {
                        effect_frame_queue.current_frame_pos = 1;
                    }
                    texdir = skin.slide_hit_effect_dir;
                }
            }
            texdir.push_back('/');
            texdir.append(std::to_string(effect_frame_queue.current_frame_pos));
            texdir.append(".png");

            ++effect_frame_queue.current_frame_pos;

            auto &effect_tex = texture_full_map[texdir];
            auto w =
                effect_tex->width *
                (editor->ebuffer.object_size_scale *
                 working_map->project_reference->config.object_width_ratio) *
                0.8;
            auto h =
                effect_tex->height *
                (editor->ebuffer.object_size_scale *
                 working_map->project_reference->config.object_height_ratio) *
                0.8;
            effect_params.func_type = FunctionType::MRECT;
            effect_params.xpos = effect_frame_queue.xpos - w / 2.0;
            effect_params.ypos = effect_frame_queue.ypos - h / 2.0;
            effect_params.width = w;
            effect_params.height = h;
            effect_params.texture = effect_tex;
            effect_params.r = 255;
            effect_params.g = 182;
            effect_params.b = 193;
            effect_params.a = 240;
            effect_params.is_volatile = true;

            // 已播放时间
            effect_frame_queue.time_left -= actual_update_time;
        }
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
void MapWorkspaceCanvas::push_shape(BufferWrapper *current_back_buffer) {
    // 绘制背景
    draw_background(current_back_buffer);

    if (working_map) {
        // 绘制轨道背景
        draw_orbits(current_back_buffer);

        std::lock_guard<std::mutex> lock(working_map->hitobjects_mutex);

        // 生成区域信息-无图形数据
        areagenerator->generate();

        // 绘制时间线
        if (editor->csettings.show_timeline) {
            draw_beats(current_back_buffer);
        }

        // 绘制判定线
        draw_judgeline(current_back_buffer);

        // 绘制特效帧
        draw_effect_frame(current_back_buffer);

        {
            std::lock_guard<std::mutex> lock(
                editor->ebuffer.selected_hitobjects_mtx);
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
            draw_hitobject(current_back_buffer, editor->ebuffer.buffer_objects,
                           HitObjectEffect::NORMAL);
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
            draw_hitobject(current_back_buffer, editbuffers,
                           HitObjectEffect::SHADOW);

            // 绘制预览区域
            draw_preview_content(current_back_buffer);
        }

        // 绘制选中区域
        draw_select_bound(current_back_buffer);

        {
            std::lock_guard<std::mutex> lock(
                editor->ebuffer.selected_timingss_mts);
            // 绘制信息区域
            draw_infoarea(current_back_buffer);
        }

        // 绘制顶部栏
        draw_top_bar(current_back_buffer);
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
        working_map->project_reference->canvas_ref = this;
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
