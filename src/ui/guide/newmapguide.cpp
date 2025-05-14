#include "newmapguide.h"

#include <qdir.h>

#include <QFileDialog>
#include <filesystem>

#include "../../util/mutil.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "ui_newmapguide.h"

NewMapGuide::NewMapGuide(QWidget *parent)
    : QDialog(parent), ui(new Ui::NewMapGuide) {
    ui->setupUi(this);

    // 设置输入框mask
    // ascii mask
    QRegularExpression asciiRegExp(R"([\x00-\x7F]+)");

    asciiValidator = new QRegularExpressionValidator(asciiRegExp, this);
    ui->title_edit->setValidator(asciiValidator);
    ui->artist_edit->setValidator(asciiValidator);

    // double mask
    doubleValidator = new QDoubleValidator(this);
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->pbpm_edit->setValidator(doubleValidator);

    // 纯数字 mask
    intValidator = new QIntValidator(this);
    ui->maplength_edit->setValidator(intValidator);
    ui->keys_edit->setValidator(intValidator);
}

NewMapGuide::~NewMapGuide() { delete ui; }

// 使用音频元数据按钮事件
void NewMapGuide::on_use_music_titleunicode_button_clicked() {
    if (!is_audio_loaded) {
        if (BackgroundAudio::loadin_audio(
                ui->music_path_edit->text().toStdString()) != -1) {
            is_audio_loaded = true;
        } else {
            XERROR(QString("加载音频[%1]失败")
                       .arg(ui->music_path_edit->text())
                       .toStdString());
            return;
        }
    }

    // 确保已成功加载音频
    ui->title_unicode_edit->setText(
        QString::fromStdString(BackgroundAudio::get_audio_title(
            ui->music_path_edit->text().toStdString())));
}

void NewMapGuide::on_use_music_artistunicode_button_clicked() {
    if (!is_audio_loaded) {
        if (BackgroundAudio::loadin_audio(
                ui->music_path_edit->text().toStdString()) != -1) {
            is_audio_loaded = true;
        } else {
            XERROR(QString("加载音频[%1]失败")
                       .arg(ui->music_path_edit->text())
                       .toStdString());
            return;
        }
    }

    // 确保已成功加载音频
    ui->artist_unicode_edit->setText(
        QString::fromStdString(BackgroundAudio::get_audio_artist(
            ui->music_path_edit->text().toStdString())));
}

// 编辑事件
// 音频路径变化
void NewMapGuide::on_music_path_edit_textChanged(const QString &arg1) {
    music_path = arg1.toStdString();
    is_audio_loaded = false;
}

void NewMapGuide::on_title_edit_textChanged(const QString &arg1) {
    title = arg1.toStdString();
}

void NewMapGuide::on_title_unicode_edit_textChanged(const QString &arg1) {
    title_unicode = arg1.toStdString();
}

void NewMapGuide::on_artist_edit_textChanged(const QString &arg1) {
    artist = arg1.toStdString();
}

void NewMapGuide::on_artist_unicode_edit_textChanged(const QString &arg1) {
    artist_unicode = arg1.toStdString();
}

void NewMapGuide::on_bg_path_edit_textChanged(const QString &arg1) {
    bg_path = arg1.toStdString();
}

void NewMapGuide::on_pbpm_edit_textChanged(const QString &arg1) {
    pbpm = arg1.toDouble();
}

void NewMapGuide::on_author_edit_textChanged(const QString &arg1) {
    author = arg1.toStdString();
}

void NewMapGuide::on_maplength_edit_textChanged(const QString &arg1) {
    map_length = arg1.toInt();
}

void NewMapGuide::on_mapversion_edit_textChanged(const QString &arg1) {
    version = arg1.toStdString();
}

void NewMapGuide::on_keys_edit_textChanged(const QString &arg1) {
    orbits = arg1.toInt();
}

void NewMapGuide::on_read_musiclength_button_clicked() {
    // 使用音频时长
    if (!is_audio_loaded) {
        if (BackgroundAudio::loadin_audio(
                ui->music_path_edit->text().toStdString()) != -1) {
            is_audio_loaded = true;
        } else {
            XERROR(QString("加载音频[%1]失败")
                       .arg(ui->music_path_edit->text())
                       .toStdString());
            return;
        }
    }

    // 确保已成功加载音频
    ui->maplength_edit->setText(
        QString::number(BackgroundAudio::get_audio_length(
            ui->music_path_edit->text().toStdString())));
}

void NewMapGuide::on_music_path_browser_button_clicked() {
    auto options = QFileDialog::DontUseNativeDialog;
    auto fileName = QFileDialog::getOpenFileName(
        this, tr("Select Music"), XLogger::last_select_directory,
        tr("audio file(*.mp3 *.ogg *.wav)"), nullptr, options);

    ui->music_path_edit->setText(fileName);
    std::filesystem::path path(fileName.toStdString());

    if (std::filesystem::exists(path)) {
        XLogger::last_select_directory =
            QDir(path.parent_path()).absolutePath();
    }
}

void NewMapGuide::on_bg_path_browser_button_clicked() {
    auto options = QFileDialog::DontUseNativeDialog;
    auto fileName = QFileDialog::getOpenFileName(
        this, tr("Select Image"), XLogger::last_select_directory,
        tr("background image(*.jpg *.png *.jpeg)"), nullptr, options);
    ui->bg_path_edit->setText(fileName);
    std::filesystem::path path(fileName.toStdString());

    if (std::filesystem::exists(path)) {
        XLogger::last_select_directory =
            QDir(path.parent_path()).absolutePath();
    }
}
