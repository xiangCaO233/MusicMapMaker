#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

#include "AudioGraphicWidget.h"

void AudioController::on_graphtype_selection_currentIndexChanged(
    [[maybe_unused]] int index) {
    ui->main_graph->set_graph_type(ui->graphtype_selection->currentData()
                                       .value<AudioGraphicWidget::GraphType>());
}

void AudioController::on_area_scale_spinner_valueChanged(int arg1) {
    ui->main_graph->set_visibleFrameRange(arg1 * 1024);
}

void AudioController::on_lock_ptr_pos_button_toggled(bool checked) {
    ui->main_graph->set_follow_playback(checked);
    ui->ptr_pos_value_spinner->setEnabled(checked);
}

void AudioController::on_ptr_pos_value_spinner_valueChanged(double arg1) {
    ui->main_graph->set_followPositionRatio(arg1);
}
