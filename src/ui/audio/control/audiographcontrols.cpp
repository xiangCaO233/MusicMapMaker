#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

#include "AudioGraphicWidget.h"

void AudioController::on_graphtype_selection_currentIndexChanged(
    [[maybe_unused]] int index) {
    ui->main_graph->set_graph_type(
        ui->graphtype_selection->currentData().value<GraphType>());
}

void AudioController::on_live_button_toggled(bool checked) {
    ui->main_graph->set_live(checked);
}
