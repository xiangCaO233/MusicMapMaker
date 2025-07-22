#include "trackmanager.h"

#include <qtmetamacros.h>

#include <QCloseEvent>
#include <ice/out/play/sdl/SDLPlayer.hpp>

#include "ui_trackmanager.h"

TrackManager::TrackManager(QWidget* parent)
    : QWidget(parent), ui(new Ui::TrackManager) {
    ui->setupUi(this);

    // 初始化sdl播放后端
    ice::SDLPlayer::init_backend();

    auto devices = ice::SDLPlayer::list_devices();
    for (const auto& deviceinfo : devices) {
        ui->device_selection->addItem(QString::fromStdString(deviceinfo.name),
                                      deviceinfo.id);
    }
}

TrackManager::~TrackManager() {
    delete ui;

    // 退出sdl后端
    ice::SDLPlayer::quit_backend();

    qDebug() << "TrackManager deleted";
}

// 把关闭事件改为hide
void TrackManager::closeEvent(QCloseEvent* event) {
    hide();
    emit close_signal();
    event->ignore();
}
