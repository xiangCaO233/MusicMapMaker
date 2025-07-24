#include "audiocontroller.h"

#include <qdir.h>
#include <qlogging.h>
#include <qtmetamacros.h>

#include <audio/callback/AudioPlayCallBack.hpp>
#include <chrono>
#include <memory>

#include "ui_audiocontroller.h"

AudioController::AudioController(QWidget* parent)
    : HideableToolWindow(parent), ui(new Ui::AudioController) {
    ui->setupUi(this);
    // 初始化单位选择器
    // 填充单位选择框
    ui->unit_selection->addItem(
        "ms", QVariant::fromValue(PositionUnit::Milliseconds));
    ui->unit_selection->addItem(
        "us", QVariant::fromValue(PositionUnit::Microseconds));
    ui->unit_selection->addItem("ns",
                                QVariant::fromValue(PositionUnit::Nanoseconds));
    ui->unit_selection->addItem("frame",
                                QVariant::fromValue(PositionUnit::Frame));
    ui->unit_selection->addItem("min:s",
                                QVariant::fromValue(PositionUnit::MinSec));
    ui->unit_selection->addItem("min:s.ms",
                                QVariant::fromValue(PositionUnit::MinSecMs));
    ui->unit_selection->addItem(
        "min:s.ms.us.ns", QVariant::fromValue(PositionUnit::MinSecMsUsNs));

    // 默认选最精确
    ui->unit_selection->setCurrentText("min:s.ms.us.ns");

    // 初始化回调
    callback = std::make_shared<PlayPosCallBack>(this);
    // 连接回调更新信号
    connect(std::static_pointer_cast<PlayPosCallBack>(callback).get(),
            &PlayPosCallBack::update_framepos, this,
            &AudioController::updateDisplayPosition);

    connect(std::static_pointer_cast<PlayPosCallBack>(callback).get(),
            &PlayPosCallBack::update_timepos, this,
            &AudioController::updateDisplayPosition);
}

// 更新显示位置
void AudioController::updateDisplayPosition() {
    if (!source_node) return;

    // 获取当前选择的单位枚举值
    auto unit = ui->unit_selection->currentData().value<PositionUnit>();
    QString displayText;

    switch (unit) {
        case PositionUnit::Milliseconds:
            displayText = QString::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    uitime_pos)
                    .count());
            break;
        case PositionUnit::Microseconds:
            displayText = QString::number(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    uitime_pos)
                    .count());
            break;
        case PositionUnit::Nanoseconds:
            displayText = QString::number(uitime_pos.count());
            break;
        case PositionUnit::Frame:
            displayText = QString::number(uiframe_pos);
            break;
        case PositionUnit::MinSec: {
            size_t total_seconds =
                std::chrono::duration_cast<std::chrono::seconds>(uitime_pos)
                    .count();
            size_t seconds = total_seconds % 60;
            size_t minutes = total_seconds / 60;
            displayText = QString("%1:%2")
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(seconds, 2, 10, QChar('0'));
            break;
        }
        case PositionUnit::MinSecMs: {
            size_t total_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    uitime_pos)
                    .count();
            size_t ms = total_ms % 1000;
            size_t total_seconds = total_ms / 1000;
            size_t seconds = total_seconds % 60;
            size_t minutes = total_seconds / 60;
            displayText = QString("%1:%2.%3")
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(seconds, 2, 10, QChar('0'))
                              .arg(ms, 3, 10, QChar('0'));
            break;
        }
        case PositionUnit::MinSecMsUsNs: {
            size_t ns_part = uitime_pos.count() % 1000;
            size_t total_us =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    uitime_pos)
                    .count();
            size_t us_part = total_us % 1000;
            size_t total_ms = total_us / 1000;
            size_t ms_part = total_ms % 1000;
            size_t total_seconds = total_ms / 1000;
            size_t seconds_part = total_seconds % 60;
            size_t minutes_part = total_seconds / 60;
            displayText = QString("%1:%2.%3.%4.%5")
                              .arg(minutes_part, 2, 10, QChar('0'))
                              .arg(seconds_part, 2, 10, QChar('0'))
                              .arg(ms_part, 3, 10, QChar('0'))
                              .arg(us_part, 3, 10, QChar('0'))
                              .arg(ns_part, 3, 10, QChar('0'));
            break;
        }
    }

    std::chrono::duration<double> total_duration_sec =
        source_node->total_time();

    double progress = 0.0;
    if (total_duration_sec.count() > 0) {
        // 将当前时间也转换为 double 秒
        std::chrono::duration<double> current_duration_sec = uitime_pos;
        progress = current_duration_sec.count() / total_duration_sec.count();
    }

    // 更新进度条
    ui->audio_progress->setValue(int(progress * ui->audio_progress->maximum()));

    // 只有在用户没有编辑时才更新，防止干扰输入
    if (!ui->time_edit->hasFocus()) {
        ui->time_edit->setText(displayText);
    }
}

AudioController::~AudioController() { delete ui; }

void AudioController::set_audio_track(
    const std::shared_ptr<ice::AudioTrack>& track) {
    // 相同无视
    if (audio_track == track) return;

    audio_track = track;
    // 更新track label 为音频title + artist
    auto title = track->get_media_info().title;
    auto artist = track->get_media_info().artist;
    QString track_label;
    if (title == "" || artist == "") {
        // 无法拼接元数据-使用文件名
        track_label = QDir(QString::fromStdString(track->path())).dirName();
    } else {
        // 拼接元数据
        track_label = QString("%1 - %2").arg(artist).arg(title);
    }
    ui->main_track_label->setText(track_label);

    // 通过track创建基本sourcenode
    source_node = std::make_shared<ice::SourceNode>(track);

    // 设置回调
    source_node->add_playcallback(callback);

    // 使用sourcenode作为基本输出
    output_node = source_node;

    // 发送更新输出节点信号
    emit update_output_node(this, nullptr, output_node);
}

void AudioController::set_item(QStandardItem* item) { refitem = item; }
