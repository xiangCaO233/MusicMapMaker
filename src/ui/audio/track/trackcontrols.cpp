#include <audio/control/audiocontroller.h>
#include <audio/track/trackmanager.h>
#include <ui_trackmanager.h>

#include <util/mutil.hpp>

// 创建轨道控制器
decltype(TrackManager::audio_controllers.begin()) TrackManager::makeController(
    const std::shared_ptr<ice::AudioTrack>& track, QStandardItem* refitem) {
    // 新建一个控制器
    auto controller = new AudioController();
    controller->set_item(refitem);
    auto controller_it = audio_controllers.emplace(
        QString::fromStdString(track->path()), controller);

    // 连接控制器的信号到轨道管理器(实际的音频上下文管理在轨道管理器)
    // 关闭控制器(实际为隐藏)
    connect(controller, &AudioController::close_signal, this,
            &TrackManager::onControllerHide);
    // 更新输出节点
    connect(controller, &AudioController::update_output_node, this,
            &TrackManager::onControllerOutputUpdate);

    // 更新轨道
    controller->set_audio_track(track);

    return controller_it;
}

void TrackManager::onControllerOutputUpdate(
    [[maybe_unused]] const AudioController* controller,
    std::shared_ptr<ice::IAudioNode> oldnode,
    std::shared_ptr<ice::IAudioNode> newnode) {
    // 移除旧的,设置新的
    mixbus->remove_source(oldnode);
    mixbus->add_source(newnode);
}

void TrackManager::onItemChanged(QStandardItem* item) {
    Qt::CheckState state = item->checkState();
    auto track =
        item->data(Qt::UserRole + 1).value<std::shared_ptr<ice::AudioTrack>>();

    // 检查控制器是否初始化过
    auto audio_path = QString::fromStdString(track->path());
    auto controller_it = audio_controllers.find(audio_path);
    if (controller_it != audio_controllers.end()) {
        // 已有则直接设置是否可见
        controller_it.value()->setVisible(state == Qt::Checked);
    } else {
        // 新建并设置可见性
        // 新建一个控制器
        controller_it = makeController(track, item);
        controller_it.value()->setVisible(state == Qt::Checked);
    }
}

// 收到控制器被关闭的信号
void TrackManager::onControllerHide(HideableToolWindow* wptr) const {
    auto controller = qobject_cast<AudioController*>(wptr);

    if (controller->item()) {
        controller->item()->setCheckState(Qt::Unchecked);
    }
}

void TrackManager::on_add_track_button_clicked() {
    auto audio_files =
        mutil::getOpenFiles(this, "导入音频",
                            {{tr("MP3 Music"), ".mp3"},
                             {tr("FLAC LosslessMusic"), ".flac"},
                             {tr("OGG PureAudio"), ".ogg"},
                             {tr("WAV WaveAudio"), ".wav"},
                             {tr("All Audio"), ".mp3 *.flac *.ogg *.wav"}});

    for (const auto& audio_file : audio_files) {
        qDebug() << "selecte:" << audio_file;
        loadin_audio(audio_file);
    }
}

void TrackManager::on_remove_track_button_clicked() {
    if (!ui->track_list->selectionModel()->hasSelection()) {
        qDebug() << "no selection";
        return;
    }
    // 先清除表格内容
    auto model = qobject_cast<QStandardItemModel*>(ui->track_list->model());
    ui->metadata_table->item(0, 1)->setText("");
    ui->metadata_table->item(1, 1)->setText("");
    ui->metadata_table->item(2, 1)->setText("");
    ui->metadata_table->item(3, 1)->setText("");
    ui->metadata_table->item(4, 1)->setText("");
    ui->metadata_table->item(5, 1)->setText("");

    // 清除专辑封面显示
    ui->album_cover->clear();
    ui->album_info_widget->hide();

    // 检查控制器是否初始化过
    auto audio_path = QString::fromStdString(
        ui->track_list->model()
            ->data(ui->track_list->currentIndex(), Qt::UserRole + 1)
            .value<std::shared_ptr<ice::AudioTrack>>()
            ->path());

    // 如果有控制器
    if (auto controller_it = audio_controllers.find(audio_path);
        controller_it != audio_controllers.end()) {
        // 移除混音输入源
        mixbus->remove_source(controller_it.value()->output());

        // 移除控制器映射
        audio_controllers.erase(controller_it);

        // 在此释放
        controller_it.value()->deleteLater();
    }

    // 移除列表项
    ui->track_list->model()->removeRow(
        ui->track_list->selectionModel()->selectedIndexes().first().row());

    // 最后移除句柄
    auto track_it = audio_tracks.find(audio_path);
    if (track_it != audio_tracks.end()) {
        audio_tracks.erase(track_it);
    }
}

void TrackManager::on_open_controller_button_clicked() {
    if (!ui->track_list->selectionModel()->hasSelection()) {
        qDebug() << "no selection";
        return;
    }

    // 获取选中的item
    auto model = qobject_cast<QStandardItemModel*>(ui->track_list->model());
    auto item = model->itemFromIndex(
        ui->track_list->selectionModel()->selectedIndexes().first());

    auto track =
        item->data(Qt::UserRole + 1).value<std::shared_ptr<ice::AudioTrack>>();
    // 选中的音频对应的路径

    auto audio_path = QString::fromStdString(track->path());
    auto state = item->checkState();

    // 检查是否有控制器
    auto controller_it = audio_controllers.find(audio_path);
    if (controller_it != audio_controllers.end()) {
        // 如果有控制器
        controller_it.value()->setVisible(true);
    } else {
        // 新建一个控制器
        controller_it = makeController(track, item);
    }
    // 设置为选中状态
    item->setCheckState(Qt::Checked);
}

void TrackManager::on_track_list_clicked(const QModelIndex& index) {
    // 点击-选中-展示歌曲元数据
    auto model = qobject_cast<QStandardItemModel*>(ui->track_list->model());
    auto data = model->data(index, Qt::UserRole + 1)
                    .value<std::shared_ptr<ice::AudioTrack>>();
    auto mediainfo = data->get_media_info();

    ui->metadata_table->item(0, 1)->setText(
        QString::fromStdString(mediainfo.title));
    ui->metadata_table->item(1, 1)->setText(
        QString::fromStdString(mediainfo.artist));
    ui->metadata_table->item(2, 1)->setText(
        QString::fromStdString(mediainfo.album));
    ui->metadata_table->item(3, 1)->setText(QString::number(mediainfo.bitrate));

    // 计算时间
    auto time_seconds =
        double(mediainfo.frame_count) / double(mediainfo.format.samplerate);
    auto timestr = QString::number(time_seconds / 60.0) + tr("min") + " | " +
                   QString::number(time_seconds) + tr("s") + " | " +
                   QString::number(time_seconds * 1000) + tr("ms");
    ui->metadata_table->item(4, 1)->setText(timestr);

    ui->metadata_table->item(5, 1)->setText(
        QString::number(mediainfo.frame_count));

    if (mediainfo.cover.isValid()) {
        QPixmap pix;
        pix.loadFromData(mediainfo.cover.data, mediainfo.cover.size);
        ui->album_cover->setImage(pix);
        ui->album_info_widget->show();
    }
}

void TrackManager::on_track_list_doubleClicked(const QModelIndex& index) {
    // 双击直接调用打开按钮函数
    on_open_controller_button_clicked();
}
