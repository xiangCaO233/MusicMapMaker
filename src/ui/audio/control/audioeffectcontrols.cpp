#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

// 变速
void AudioController::on_stretcher_speed_slider_valueChanged(int value) {
    // 速率调整条
    auto percent_value = double(value) / 100.0;
    // 设置 ui->speed_value_spinner 的值之前，阻塞信号
    {
        QSignalBlocker blocker(ui->speed_value_spinner);
        ui->speed_value_spinner->setValue(percent_value);
    }
    // blocker 离开作用域，spinner 的信号恢复

    auto ratio = percent_value / 100.0;
    process_chain->stretcher->set_playback_ratio(ratio);
    // 同步图形组件的处理链
    ui->main_graph->chain()->stretcher->set_playback_ratio(ratio);
}

void AudioController::on_speed_value_spinner_valueChanged(double arg1) {
    // 速率调整spinner
    // 设置 ui->stretcher_speed_slider 的值之前，阻塞信号
    {
        QSignalBlocker blocker(ui->stretcher_speed_slider);
        ui->stretcher_speed_slider->setValue(
            static_cast<int>(std::round(arg1 * 100.0)));
    }
    // blocker 离开作用域，slider 的信号恢复

    auto ratio = arg1 / 100.0;
    process_chain->stretcher->set_playback_ratio(ratio);
    // 同步图形组件的处理链
    ui->main_graph->chain()->stretcher->set_playback_ratio(ratio);
}

void AudioController::on_reset_stretcher_button_clicked() {
    ui->stretcher_speed_slider->setValue(100);
}

// 变调
// 更新音调变化
void AudioController::update_pitch(double semitones) {
    // 根据半音计算倍率
    double scale = std::pow(2.0, semitones / 12.0);

    // 阻塞所有四个相关UI组件的信号，防止循环触发
    QSignalBlocker blocker1(ui->semitones_slider);
    QSignalBlocker blocker2(ui->semitones_spinner);
    QSignalBlocker blocker3(ui->scale_slider);
    QSignalBlocker blocker4(ui->scale_spinner);

    // 更新UI组件的值
    // 半音滑块的范围是 semitones * 100 -240~240
    ui->semitones_slider->setValue(
        static_cast<int>(std::round(semitones * 100.0)));
    ui->semitones_spinner->setValue(semitones);

    // 假设倍率滑块的范围是 scale * 100 (例如 25 到 400)
    ui->scale_slider->setValue(static_cast<int>(std::round(scale * 100.0)));
    ui->scale_spinner->setValue(scale);

    // 将音高变化应用到处理链
    // 建议使用 set_pitch_shift，因为它会同时更新内部的 semitones 和 scale
    process_chain->pitchshifter->set_pitch_shift(semitones);
    // 同步图形组件的处理链
    ui->main_graph->chain()->pitchshifter->set_pitch_shift(semitones);
}
void AudioController::on_reset_pitch_shift_button_clicked() {
    update_pitch(0.);
}
void AudioController::on_semitones_slider_valueChanged(int value) {
    // 半音调整条: 从整数值计算出 double 类型的半音值
    // (对应 update_pitch 中的 * 10.0)
    double semitones = static_cast<double>(value) / 100.0;
    update_pitch(semitones);
}
void AudioController::on_semitones_spinner_valueChanged(double arg1) {
    // 半音调整spinner
    // 半音调整spinner: 直接使用其 double 值
    update_pitch(arg1);
}

void AudioController::on_scale_slider_valueChanged(int value) {
    // 倍率调整条: 从整数值计算出 double 类型的倍率值
    // (对应 update_pitch 中的 * 100.0)
    double scale = static_cast<double>(value) / 100.0;
    // 防止因值为0或负数导致 log2 计算错误
    if (scale <= 0) return;
    // 从倍率反向计算半音值
    double semitones = 12.0 * std::log2(scale);
    update_pitch(semitones);
}

void AudioController::on_scale_spinner_valueChanged(double arg1) {
    // 倍率调整spinner: 直接使用其 double 值
    // 防止因值为0或负数导致 log2 计算错误
    if (arg1 <= 0) return;
    // 从倍率反向计算半音值
    double semitones = 12.0 * std::log2(arg1);
    update_pitch(semitones);
}
