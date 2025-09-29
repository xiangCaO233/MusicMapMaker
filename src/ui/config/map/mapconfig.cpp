#include <mapconfig.h>
#include <trackmanager.h>
#include <ui_mapconfig.h>

#include <mmm/map/MMap.hpp>

MapConfig::MapConfig(MMap *map, QWidget *parent)
    : QWidget(parent), ui(new Ui::MapConfig), map_ref(map) {
    ui->setupUi(this);
    // 移除窗口修饰
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    // 初始化各元数据输入组件
    auto &basemeta = map_ref->base_metadata();

    // 音频
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
}

MapConfig::~MapConfig() { delete ui; }

// 检查完成性
void MapConfig::update_ifcompeleted() {
    auto &basemeta = map_ref->base_metadata();
    bool compeleted{true};
    if (basemeta.main_audio_path.empty()) compeleted = false;
    if (basemeta.title.empty()) compeleted = false;
    if (basemeta.title_unicode.empty()) compeleted = false;
    if (basemeta.artist.empty()) compeleted = false;
    if (basemeta.artist_unicode.empty()) compeleted = false;
    if (basemeta.preference_bpm <= 0) compeleted = false;
    if (basemeta.track_count <= 0) compeleted = false;
    if (basemeta.map_length <= 0) compeleted = false;
    if (basemeta.main_cover_path.empty()) compeleted = false;
    if (basemeta.author.empty()) compeleted = false;
    if (basemeta.version.empty()) compeleted = false;

    this->compeleted = compeleted;
}

void MapConfig::update_audio_tracklist(TrackManager *trackmanager) {
    this->trackmanager = trackmanager;
    ui->audio_track_selectbox->clear();
    auto maintracks = trackmanager->get_maintrack();
    for (const auto &trackname : maintracks) {
        ui->audio_track_selectbox->addItem(trackname);
    }
    // 更新到当前选中项
    QSignalBlocker blocker(ui->audio_track_selectbox);
    auto index = find_track_indexinCombobox();
    if (index >= 0) {
        ui->audio_track_selectbox->setCurrentIndex(index);
    }
}

// 检查音频轨道位置
int MapConfig::find_track_indexinCombobox() {
    return ui->audio_track_selectbox->findText(QString::fromStdString(
        map_ref->base_metadata().main_audio_path.generic_string()));
}
