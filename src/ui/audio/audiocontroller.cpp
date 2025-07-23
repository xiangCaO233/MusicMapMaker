#include "audiocontroller.h"

#include "ui_audiocontroller.h"

AudioController::AudioController(QWidget* parent)
    : HideableToolWindow(parent), ui(new Ui::AudioController) {
    ui->setupUi(this);
}

AudioController::~AudioController() { delete ui; }
