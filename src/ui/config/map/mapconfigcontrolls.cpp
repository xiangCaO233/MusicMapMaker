#include <colorful-log.h>
#include <mapconfig.h>
#include <trackmanager.h>
#include <ui_mapconfig.h>

#include <filesystem>
#include <ice/core/SourceNode.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/project/MProject.hpp>
#include <util/mutil.hpp>

void MapConfig::on_confirm_button_clicked() {
    // 应用修改
    auto& basemeta = map_ref->base_metadata();
    basemeta.main_audio_path =
        std::filesystem::path(ui->audio_path_lineedit->text().toStdString());
    basemeta.title = ui->title_ascii_lineedit->text().toStdString();
    basemeta.title_unicode = ui->title_unicode_lineedit->text().toStdString();
    basemeta.artist = ui->artist_ascii_lineedit->text().toStdString();
    basemeta.artist_unicode = ui->artist_unicode_lineedit->text().toStdString();
    basemeta.preference_bpm = ui->preferencebpm_lineedit->text().toDouble();
    basemeta.track_count = ui->trackcount_lineedit->text().toInt();
    basemeta.map_length = ui->map_length_lineedit->text().toInt();
    basemeta.main_cover_path = ui->cover_lineedit->text().toStdString();
    basemeta.author = ui->author_lineedit->text().toStdString();
    basemeta.version = ui->version_lineedit->text().toStdString();
    basemeta.name = ui->mapname_lineedit->text().toStdString();

    update_components_frommeta();

    // 更新完整性
    update_ifcompeleted();

    hide();
}

void MapConfig::on_cancel_button_clicked() {
    // 取消修改直接恢复数据为当前已有的元数据
    auto& basemeta = map_ref->base_metadata();
    if (!basemeta.main_audio_path.empty()) {
        ui->audio_path_lineedit->setText(
            QString::fromStdString(basemeta.main_audio_path.generic_string()));
    }
    if (!basemeta.title.empty()) {
        ui->title_ascii_lineedit->setText(
            QString::fromStdString(basemeta.title));
    }
    if (!basemeta.title_unicode.empty()) {
        ui->title_unicode_lineedit->setText(
            QString::fromStdString(basemeta.title_unicode));
    }
    if (!basemeta.artist.empty()) {
        ui->artist_ascii_lineedit->setText(
            QString::fromStdString(basemeta.artist));
    }
    if (!basemeta.artist_unicode.empty()) {
        ui->artist_unicode_lineedit->setText(
            QString::fromStdString(basemeta.artist_unicode));
    }

    if (basemeta.preference_bpm > 0) {
        ui->preferencebpm_lineedit->setText(
            QString::number(basemeta.preference_bpm, 'f', 3));
    }

    if (basemeta.track_count > 0) {
        ui->trackcount_lineedit->setText(QString::number(basemeta.track_count));
    }

    if (basemeta.map_length > 0) {
        ui->map_length_lineedit->setText(QString::number(basemeta.map_length));
    }

    if (!basemeta.main_cover_path.empty()) {
        ui->cover_lineedit->setText(
            QString::fromStdString(basemeta.main_cover_path.generic_string()));
    }

    if (!basemeta.author.empty()) {
        ui->author_lineedit->setText(QString::fromStdString(basemeta.author));
    }

    if (!basemeta.version.empty()) {
        ui->version_lineedit->setText(QString::fromStdString(basemeta.version));
    }

    if (!basemeta.name.empty()) {
        ui->mapname_lineedit->setText(QString::fromStdString(basemeta.name));
    }
    // 然后隐藏窗口
    hide();
}

void MapConfig::on_load_audio_immediately_button_clicked() {
    // 当场载入音频为音轨
    std::filesystem::path audio_path(
        ui->audio_path_lineedit->text().toStdString());
    if (std::filesystem::exists(audio_path)) {
        if (auto track = trackmanager
                             ->get_track(QString::fromStdString(
                                 audio_path.generic_string()))
                             .lock()) {
            XWARN("音轨已加载过");
            selected_track = track;
        } else {
            auto wtrack =
                trackmanager->loadBack(audio_path.generic_string(), true);
            // 刷新音轨列表
            // 会立即选中combobox
            update_audio_tracklist(trackmanager);
            selected_track = wtrack.lock();
        }
        // map_ref->project()->add_audio_track(audio_path.generic_string(),
        //                                     selected_track, true);
    } else {
        XWARN("音频文件不存在");
    }
}

void MapConfig::on_audio_track_selectbox_currentTextChanged(
    const QString& arg1) {
    // 修改音频路径为选中项
    ui->audio_path_lineedit->setText(arg1);
    // 获取音轨
    selected_track = trackmanager->get_track(arg1).lock();
}

void MapConfig::on_probe_audio_title_button_clicked() {
    // 使用当前选中的音轨获取音频标题
    ui->title_unicode_lineedit->setText(
        QString::fromStdString(selected_track->get_media_info().title));
}

void MapConfig::on_probe_audio_artist_button_clicked() {
    // 使用当前选中的音轨获取音频艺术家
    ui->artist_unicode_lineedit->setText(
        QString::fromStdString(selected_track->get_media_info().artist));
}

void MapConfig::on_probe_audio_length_button_clicked() {
    // 使用当前选中的音轨获取音频时长
    auto sourcenode = std::make_unique<ice::SourceNode>(selected_track);
    ui->map_length_lineedit->setText(QString::number(
        uint32_t(double(sourcenode->total_time().count() / 1000.) / 1000.)));
}

void MapConfig::on_audio_file_browser_button_clicked() {
    // 选择音频文件按钮
    auto audiofile = mutil::getOpenFile(this, tr("Select Audio File"),
                                        {{tr("Audio File"), ".mp3 .wav .ogg"}},
                                        XLogger::last_select_directory);
    if (!audiofile.isEmpty()) {
        std::filesystem::path selectpath(audiofile.toStdString());
        if (std::filesystem::exists(selectpath)) {
            XLogger::last_select_directory =
                QDir(selectpath.parent_path()).absolutePath();
        }
        ui->audio_path_lineedit->setText(audiofile);
    }
}

void MapConfig::on_cover_file_browser_button_clicked() {
    // 选择封面文件按钮
    auto coverfile = mutil::getOpenFile(this, tr("Select Image File"),
                                        {{tr("Image File"), ".jpg .png .jpeg"}},
                                        XLogger::last_select_directory);
    if (!coverfile.isEmpty()) {
        std::filesystem::path selectpath(coverfile.toStdString());
        if (std::filesystem::exists(selectpath)) {
            XLogger::last_select_directory =
                QDir(selectpath.parent_path()).absolutePath();
        }
        ui->cover_lineedit->setText(coverfile);
    }
}
