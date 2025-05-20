#include "timecontroller.h"

#include <qlogging.h>
#include <qtmetamacros.h>

#include <memory>
#include <string>

#include "../../mmm/MapWorkProject.h"
#include "../../mmm/map/MMap.h"
#include "../../util/mutil.h"
#include "../GlobalSettings.h"
#include "AudioManager.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "guide/newtimingguide.h"
#include "mainwindow.h"
#include "ui_timecontroller.h"

TimeController::TimeController(QWidget *parent)
    : QWidget(parent), ui(new Ui::audio_time_controller) {
    ui->setupUi(this);
}

TimeController::~TimeController() { delete ui; }

// 使用主题
void TimeController::use_theme(GlobalTheme theme) {
    current_theme = theme;
    QColor file_button_color;
    switch (theme) {
        case GlobalTheme::DARK: {
            file_button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            file_button_color = QColor(0, 0, 0);
            break;
        }
    }

    // 设置按钮图标颜色
    mutil::set_button_svgcolor(ui->pausebutton, ":/icons/play.svg",
                               file_button_color, 16, 16);
    mutil::set_button_svgcolor(ui->enablepitchaltbutton,
                               ":/icons/pitch-alt.svg", file_button_color, 16,
                               16);
    mutil::set_button_svgcolor(ui->resetspeedbutton, ":/icons/bolt.svg",
                               file_button_color, 16, 16);

    mutil::set_button_svgcolor(ui->reset_global_volume_button,
                               ":/icons/volume-high.svg", file_button_color, 16,
                               16);
    mutil::set_button_svgcolor(ui->reset_music_volume_button,
                               ":/icons/music.svg", file_button_color, 16, 16);

    mutil::set_button_svgcolor(ui->reset_effect_volume_button,
                               ":/icons/effect.svg", file_button_color, 16, 16);
}

// 更新全局音量按钮(主题)
void TimeController::update_global_volume_button() {
    QColor button_color;
    switch (current_theme) {
        case GlobalTheme::DARK: {
            button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            button_color = QColor(0, 0, 0);
            break;
        }
    }
    auto current_global_volume = ui->global_volume_slider->value();
    mutil::set_button_svgcolor(
        ui->reset_global_volume_button,
        (current_global_volume > 50
             ? ":/icons/volume-high.svg"
             : (current_global_volume > 0 ? ":/icons/volume-low.svg"
                                          : ":/icons/volume-off.svg")),
        button_color, 16, 16);
}

// 更新音频状态
void TimeController::update_audio_status() {
    if (binding_map) {
        auto file = binding_map->audio_file_abs_path.generic_string();
        std::replace(file.begin(), file.end(), '\\', '/');
        if (pause) {
            BackgroundAudio::pause_audio(
                binding_map->project_reference->devicename, file);
        } else {
            BackgroundAudio::play_audio(
                binding_map->project_reference->devicename, file);
        }
        // 添加回调
        BackgroundAudio::add_playpos_callback(
            binding_map->project_reference->devicename, file,
            binding_map->audio_pos_callback);
        // 暂停的话同步一下
        if (pause) {
            // 防止播放器已播放完暂停了死等待
            if (BackgroundAudio::get_device_playerstatus(
                    binding_map->project_reference->devicename) == 1) {
                XINFO("同步画布时间");
                auto canvas_pos =
                    binding_map->project_reference->map_canvasposes.at(
                        binding_map);
                // XWARN("画布与音频时间差:[" +
                //       std::to_string(current_audio_time - canvas_pos) + "]");
                // 同步音频时间为画布时间
                BackgroundAudio::set_audio_pos(
                    binding_map->project_reference->devicename, file,
                    canvas_pos);
                emit music_pos_synchronized(
                    canvas_pos - xutil::plannerpcmpos2milliseconds(
                                     x::Config::mix_buffer_size / 3.0,
                                     static_cast<int>(x::Config::samplerate)));
                emit binding_map->audio_pos_callback->music_play_callback(
                    canvas_pos);

                // binding_map->audio_pos_callback->waitfor_clear_buffer(
                //     [&](double current_audio_time) {
                //       //
                //       binding_map->project_reference->map_canvasposes.at(binding_map)
                //       // =
                //       //     current_audio_time;
                //     });
            }
        }
    }
}

// 更新暂停按钮
void TimeController::update_pause_button() {
    QColor button_color;
    switch (current_theme) {
        case GlobalTheme::DARK: {
            button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            button_color = QColor(0, 0, 0);
            break;
        }
    }
    mutil::set_button_svgcolor(
        ui->pausebutton, (pause ? ":/icons/play.svg" : ":/icons/pause.svg"),
        button_color, 16, 16);
}

// 暂停按钮
void TimeController::on_pausebutton_clicked() {
    pause = !pause;

    update_pause_button();
    // 切换音频播放状态
    update_audio_status();
    emit pause_button_changed(pause);
}

// 画布暂停槽
void TimeController::on_canvasPause(bool paused) {
    // 更新暂停状态和按钮图标
    pause = paused;
    // 修改timeedit可用状态
    // 非暂停时不可使用
    ui->lineEdit->setEnabled(pause);

    // 更新暂停按钮图标
    update_pause_button();

    // 切换音频播放状态
    update_audio_status();
}

// 实时信息变化槽
// bpm
void TimeController::on_currentBpmChanged(double bpm) {
    ui->bpmvalue->setText(QString::number(bpm, 'f', 2));
}

// timeline_speed
void TimeController::on_currentTimelineSpeedChanged(double timeline_speed) {
    ui->timelinespeedvalue->setText(QString::number(timeline_speed, 'f', 2));
}

// page选择了新map事件
void TimeController::on_selectnewmap(std::shared_ptr<MMap> &map) {
    binding_map = map;
    // 读取项目的设置
    // 画布配置
    auto project_owidth_value =
        int(map ? map->project_reference->config.object_width_ratio * 100 : 0);
    ui->owidth_scale_button->setText(QString::number(project_owidth_value));
    ui->owidth_scale_slider->setValue(project_owidth_value);

    auto project_oheight_value =
        int(map ? map->project_reference->config.object_height_ratio * 100 : 0);
    ui->oheight_scale_button->setText(QString::number(project_oheight_value));
    ui->oheight_scale_slider->setValue(project_oheight_value);

    auto project_timeline_zoom_value =
        int(map ? map->project_reference->config.timeline_zoom * 100 : 0);
    ui->timeline_zoom_button->setText(
        QString::number(project_timeline_zoom_value));
    ui->timeline_zoom_slider->setValue(project_timeline_zoom_value);

    // 音频配置
    auto project_global_volume =
        int(map ? map->project_reference->config.pglobal_volume * 100 : 50);
    ui->global_volume_slider->setValue(project_global_volume);
    ui->global_volume_value_label->setText(
        QString::number(project_global_volume));

    auto project_music_volume =
        int(map ? map->project_reference->config.pmusic_volume * 100 : 100);
    ui->music_volume_slider->setValue(project_music_volume);
    ui->music_volume_value_label->setText(
        QString::number(project_music_volume));

    auto project_effect_volume =
        int(map ? map->project_reference->config.peffect_volume * 100 : 100);
    ui->effect_volume_slider->setValue(project_effect_volume);
    ui->effect_volume_value_label->setText(
        QString::number(project_effect_volume));

    if (binding_map) {
        BackgroundAudio::set_global_volume(
            binding_map->project_reference->devicename,
            map->project_reference->config.pmusic_volume);
        BackgroundAudio::set_music_volume(
            binding_map->project_reference->devicename,
            map->project_reference->config.pmusic_volume);
        BackgroundAudio::set_effects_volume(
            binding_map->project_reference->devicename,
            map->project_reference->config.peffect_volume);
    }
}

// 画布时间变化事件
void TimeController::oncanvas_timestampChanged(double time) {
    if (!binding_map) return;
    // 计算进度
    auto progress = time / binding_map->map_length;
    // 更新lineedit和progress
    if (tformat == TimeFormat::MILLISECONDS) {
        ui->lineEdit->setText(QString::number(int32_t(time)));
    }
    if (tformat == TimeFormat::HHMMSSZZZ) {
        ui->lineEdit->setText(mutil::millisecondsToQString(int32_t(time)));
    }
    ui->playprogress->setValue(ui->playprogress->maximum() * progress);

    // 暂停可调
    if (!pause) return;
    auto file = QDir(binding_map->audio_file_abs_path);
    // 更新音频播放位置
    if (binding_map &&
        binding_map->project_reference->devicename != "unknown output device") {
        BackgroundAudio::set_audio_pos(
            binding_map->project_reference->devicename,
            file.canonicalPath().toStdString(), time);
    }
}

// 变速设置
void TimeController::on_doubleSpinBox_valueChanged(double arg1) {
    // TODO(xiang 2025-04-26): 实现功能
    auto speed_value = arg1 / 100.0;
    if (!ui->enablepitchaltbutton->isChecked()) {
        // 非变速可以实时调整
        // 为音频应用变速(根据是否启用变调)
        if (binding_map->project_reference->devicename ==
            "unknown output device")
            return;
        BackgroundAudio::set_play_speed(
            binding_map->project_reference->devicename, speed_value,
            ui->enablepitchaltbutton->isChecked());
        // 同步画布的时间倍率
        emit playspeed_changed(speed_value);
    }
}

// 变速设置完成
void TimeController::on_doubleSpinBox_editingFinished() {
    auto speed_value = ui->doubleSpinBox->value() / 100.0;
    qDebug() << "finished:" << speed_value;
    if (ui->enablepitchaltbutton->isChecked()) {
        // 变速不可以实时调整
        // 为音频应用变速(根据是否启用变调)
        if (binding_map->project_reference->devicename ==
            "unknown output device")
            return;
        BackgroundAudio::set_play_speed(
            binding_map->project_reference->devicename, speed_value,
            ui->enablepitchaltbutton->isChecked());
        // 同步画布的时间倍率
        emit playspeed_changed(speed_value);
    }
}

// 重置变速按钮事件
void TimeController::on_resetspeedbutton_clicked() {
    // TODO(xiang 2025-04-26): 实现功能
    // 重置音频速度为1.0
    // 同步画布的时间倍率
}

// 启用变速变调按钮事件
void TimeController::on_enablepitchaltbutton_clicked() {
    enable_pitch_alt = ui->enablepitchaltbutton->isChecked();
    BackgroundAudio::enable_pitch_alt = enable_pitch_alt;
}

// 全局音频音量slider值变化事件
void TimeController::on_global_volume_slider_valueChanged(int value) {
    if (binding_map) {
        auto volume_value = float(value) / 100.0;
        BackgroundAudio::set_global_volume(
            binding_map->project_reference->devicename, volume_value);
        // 更新标签
        ui->global_volume_value_label->setText(
            QString::number(volume_value * 100));
        // 更新项目音量配置
        binding_map->project_reference->config.pglobal_volume = volume_value;
        binding_map->project_reference->audio_volume_node.attribute("global")
            .set_value(volume_value);

        update_global_volume_button();
    }
}

// 音乐音量slider值变化事件
void TimeController::on_music_volume_slider_valueChanged(int value) {
    if (binding_map) {
        auto volume_value = float(value) / 100.0;
        BackgroundAudio::set_music_volume(
            binding_map->project_reference->devicename, volume_value);
        // 更新标签
        ui->music_volume_value_label->setText(
            QString::number(volume_value * 100));
        // 更新项目音量配置
        binding_map->project_reference->config.pmusic_volume = volume_value;
        binding_map->project_reference->audio_volume_node.attribute("music")
            .set_value(volume_value);
    }
}

// 效果音量slider值变化事件
void TimeController::on_effect_volume_slider_valueChanged(int value) {
    if (binding_map) {
        auto volume_value = float(value) / 100.0;
        BackgroundAudio::set_effects_volume(
            binding_map->project_reference->devicename, volume_value);
        // 更新标签
        ui->effect_volume_value_label->setText(
            QString::number(volume_value * 100));
        // 更新项目音量配置
        binding_map->project_reference->config.peffect_volume = volume_value;
        binding_map->project_reference->audio_volume_node.attribute("effect")
            .set_value(volume_value);
    }
}

// 静音全局按钮事件
void TimeController::on_reset_global_volume_button_clicked() {
    if (binding_map) {
        BackgroundAudio::set_global_volume(
            binding_map->project_reference->devicename, 0);
        // 更新标签
        ui->global_volume_value_label->setText(QString::number(0));
        update_global_volume_button();
    }
}

// 静音音乐按钮事件
void TimeController::on_reset_music_volume_button_clicked() {
    if (binding_map) {
        BackgroundAudio::set_music_volume(
            binding_map->project_reference->devicename, 0);
        // 更新标签
        ui->music_volume_value_label->setText(QString::number(0));
    }
}

// 静音效果按钮事件
void TimeController::on_reset_effect_volume_button_clicked() {
    if (binding_map) {
        BackgroundAudio::set_effects_volume(
            binding_map->project_reference->devicename, 0);
        // 更新标签
        ui->effect_volume_value_label->setText(QString::number(0));
    }
}

// 切换时间格式按钮事件
void TimeController::on_time_format_button_clicked() {
    if (tformat == TimeFormat::HHMMSSZZZ) {
        // 当前是HHMMSSZZZ--切换为毫秒
        ui->time_format_button->setText("ms");
        // 更新时间编辑框内容
        ui->lineEdit->setText(QString::number(
            mutil::qstringToMilliseconds(latest_time_edit_value)));
        tformat = TimeFormat::MILLISECONDS;
        return;
    }
    if (tformat == TimeFormat::MILLISECONDS) {
        // 当前是毫秒--切换为HHMMSSZZZ
        ui->time_format_button->setText("hh:mm:ss.zzz");
        // 更新时间编辑框内容
        ui->lineEdit->setText(mutil::millisecondsToQString(
            std::stoi(latest_time_edit_value.toStdString())));
        tformat = TimeFormat::HHMMSSZZZ;
    }
}

// 时间编辑框内容变化事件
void TimeController::on_lineEdit_textChanged(const QString &arg1) {
    latest_time_edit_value = arg1;
}

// 时间编辑框回车按下事件
void TimeController::on_lineEdit_returnPressed() {
    on_lineEdit_editingFinished();
}

// 时间编辑框编辑完成事件
void TimeController::on_lineEdit_editingFinished() {
    // 预先记录修改前的数据
    // 检查输入数据是否合法--不合法恢复缓存数据
    if (tformat == TimeFormat::HHMMSSZZZ) {
        auto milliseconds = mutil::qstringToMilliseconds(ui->lineEdit->text());
        if (milliseconds == -1) {
            // 格式错误
            XERROR("时间格式错误-->需要[hh:mm:ss.zzz]");
            ui->lineEdit->setText(latest_time_edit_value);
        } else {
            ui->lineEdit->setText(mutil::millisecondsToQString(milliseconds));
            oncanvas_timestampChanged(milliseconds);
            emit time_edited(milliseconds);
        }
    } else if (tformat == TimeFormat::MILLISECONDS) {
        bool isalldigits =
            mutil::isStringAllDigits_Iteration(ui->lineEdit->text());
        if (isalldigits) {
            auto timem = std::stoi(ui->lineEdit->text().toStdString());
            ui->lineEdit->setText(QString::number(timem));
            oncanvas_timestampChanged(timem);
            emit time_edited(timem);
        } else {
            // 格式错误
            XERROR("时间格式错误-->需要[0-9]");
            ui->lineEdit->setText(latest_time_edit_value);
        }
    }
}

// 物件宽度缩放调节
void TimeController::on_owidth_scale_slider_valueChanged(int value) {
    if (binding_map) {
        auto ratio_value = double(value) / 100.0;
        binding_map->project_reference->config.object_width_ratio = ratio_value;
        binding_map->project_reference->sizeconfig_node
            .attribute("objwidth-scale")
            .set_value(ratio_value);
        ui->owidth_scale_button->setText(QString::number(value));
    }
}

// 物件高度缩放调节
void TimeController::on_oheight_scale_slider_valueChanged(int value) {
    if (binding_map) {
        auto ratio_value = double(value) / 100.0;
        binding_map->project_reference->config.object_height_ratio =
            ratio_value;
        binding_map->project_reference->sizeconfig_node
            .attribute("objheight-scale")
            .set_value(ratio_value);
        ui->oheight_scale_button->setText(QString::number(value));
    }
}

// 画布调节时间线缩放
void TimeController::on_canvasAdjustTimelineZoom(int value) {
    timeline_zoom_sync_lock = true;
    ui->timeline_zoom_slider->setValue(value);
    ui->timeline_zoom_button->setText(QString::number(value));
}

// 时间线缩放调节
void TimeController::on_timeline_zoom_slider_valueChanged(int value) {
    if (timeline_zoom_sync_lock) {
        timeline_zoom_sync_lock = false;
        return;
    }
    if (binding_map) {
        auto zoom_value = double(value) / 100.0;
        binding_map->project_reference->config.timeline_zoom = zoom_value;
        binding_map->project_reference->canvasconfig_node
            .attribute("timeline-zoom")
            .set_value(zoom_value);
        ui->timeline_zoom_button->setText(QString::number(value));
    }
}

// 重置宽度缩放按钮
void TimeController::on_owidth_scale_button_clicked() {
    ui->owidth_scale_slider->setValue(100);
}

// 重置高度缩放按钮
void TimeController::on_oheight_scale_button_clicked() {
    ui->oheight_scale_slider->setValue(100);
}

// 重置时间线缩放按钮
void TimeController::on_timeline_zoom_button_clicked() {
    ui->timeline_zoom_slider->setValue(100);
}

// 新建timing按钮事件
void TimeController::on_new_timing_button_clicked() {
    if (binding_map) {
        // 在当前时间唤出新建timing向导
        NewTimingGuide dialog(binding_map);
        dialog.set_time(
            binding_map->project_reference->map_canvasposes[binding_map]);
        if (dialog.exec() == QDialog::Accepted) {
            // 确认
            XINFO("创建时间点");
            auto timing = std::make_shared<Timing>();
            timing->type = TimingType::GENERAL;
            timing->is_base_timing = !dialog.inheritance_pretiming;
            timing->timestamp = dialog.timestamp;
            timing->basebpm = dialog.bpm;
            if (timing->is_base_timing) {
                timing->bpm = timing->basebpm;
            } else {
                timing->bpm = dialog.speed;
            }
            binding_map->insert_timing(timing);
        } else {
            // 取消
            XWARN("取消创建时间点");
        }
    } else {
        XWARN("无绑定谱面");
    }
}
