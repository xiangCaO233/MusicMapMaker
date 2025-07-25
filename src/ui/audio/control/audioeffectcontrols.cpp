#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

void AudioController::on_stretcher_speed_slider_valueChanged(int value) {
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
