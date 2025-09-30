#include <mapconfig.h>
#include <trackmanager.h>
#include <ui_mapconfig.h>

#include <QDoubleValidator>
#include <QIntValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <mmm/map/MMap.hpp>

MapConfig::MapConfig(MMap *map, QWidget *parent)
    : QWidget(parent), ui(new Ui::MapConfig), map_ref(map) {
    ui->setupUi(this);
    // 移除窗口修饰
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    confirmButton = ui->confirm_button;
    cancleButton = ui->cancel_button;
    // 初始化各元数据输入组件
    auto &basemeta = map_ref->base_metadata();
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

    // 设置各自的输入过滤器
    // 只允许输入整数
    auto *int_validator = new QIntValidator(0, INT_MAX, this);
    ui->trackcount_lineedit->setValidator(int_validator);
    ui->map_length_lineedit->setValidator(int_validator);

    // 只允许输入小数
    auto *double_validator = new QDoubleValidator(0.0, 1000.0, 3, this);
    // 设置为标准小数表示法 (例如 123.456), 而不是科学计数法
    double_validator->setNotation(QDoubleValidator::StandardNotation);
    ui->preferencebpm_lineedit->setValidator(double_validator);

    // 只允许输入ASCII字符
    // 正则表达式 "^[\x20-\x7E]*$" 匹配所有可打印的ASCII字符
    QRegularExpression ascii_regex("^[\\x20-\\x7E]*$");
    auto *ascii_validator = new QRegularExpressionValidator(ascii_regex, this);
    ui->title_ascii_lineedit->setValidator(ascii_validator);
    ui->artist_ascii_lineedit->setValidator(ascii_validator);

    // 非法输入提醒
    // trackcount_lineedit
    connect(ui->trackcount_lineedit, &QLineEdit::inputRejected, this, [this]() {
        ui->trackcount_lineedit->setStyleSheet("border: 1px solid red;");
        ui->trackcount_lineedit->setToolTip(tr("Only integers are allowed."));
    });
    connect(ui->trackcount_lineedit, &QLineEdit::textChanged, this,
            [this]() { updateLineEditStatus(ui->trackcount_lineedit); });

    // map_length_lineedit
    connect(ui->map_length_lineedit, &QLineEdit::inputRejected, this, [this]() {
        ui->map_length_lineedit->setStyleSheet("border: 1px solid red;");
        ui->map_length_lineedit->setToolTip(tr("Only integers are allowed."));
    });
    connect(ui->map_length_lineedit, &QLineEdit::textChanged, this,
            [this]() { updateLineEditStatus(ui->map_length_lineedit); });

    // preferencebpm_lineedit
    connect(
        ui->preferencebpm_lineedit, &QLineEdit::inputRejected, this, [this]() {
            ui->preferencebpm_lineedit->setStyleSheet("border: 1px solid red;");
            ui->preferencebpm_lineedit->setToolTip(
                tr("Only decimal numbers are allowed."));
        });
    connect(ui->preferencebpm_lineedit, &QLineEdit::textChanged, this,
            [this]() { updateLineEditStatus(ui->preferencebpm_lineedit); });

    // title_ascii_lineedit
    connect(
        ui->title_ascii_lineedit, &QLineEdit::inputRejected, this, [this]() {
            ui->title_ascii_lineedit->setStyleSheet("border: 1px solid red;");
            ui->title_ascii_lineedit->setToolTip(
                tr("Only ASCII characters are allowed."));
        });
    connect(ui->title_ascii_lineedit, &QLineEdit::textChanged, this,
            [this]() { updateLineEditStatus(ui->title_ascii_lineedit); });

    // artist_ascii_lineedit
    connect(
        ui->artist_ascii_lineedit, &QLineEdit::inputRejected, this, [this]() {
            ui->artist_ascii_lineedit->setStyleSheet("border: 1px solid red;");
            ui->artist_ascii_lineedit->setToolTip(
                tr("Only ASCII characters are allowed."));
        });
    connect(ui->artist_ascii_lineedit, &QLineEdit::textChanged, this,
            [this]() { updateLineEditStatus(ui->artist_ascii_lineedit); });

    // 实时更新完成状态
    connect(ui->audio_path_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->title_ascii_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->title_unicode_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->artist_ascii_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->artist_unicode_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->preferencebpm_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->trackcount_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->map_length_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->cover_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->author_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
    connect(ui->version_lineedit, &QLineEdit::textChanged, this,
            &MapConfig::update_ifcompeleted_dialog);
}

MapConfig::~MapConfig() { delete ui; }

void MapConfig::use_theme(GlobalTheme theme) {}

// 根据验证状态更新 QLineEdit 的样式
void MapConfig::updateLineEditStatus(QLineEdit *lineEdit) {
    if (!lineEdit) return;

    const QValidator *validator = lineEdit->validator();
    if (!validator) {
        lineEdit->setStyleSheet("");  // 如果没有验证器，恢复默认样式
        return;
    }

    QString text = lineEdit->text();
    int pos = 0;  // 模拟光标位置
    QValidator::State state = validator->validate(text, pos);

    switch (state) {
        case QValidator::Acceptable:
            // 状态：合法。恢复正常样式。
            lineEdit->setStyleSheet("");
            lineEdit->setToolTip("");  // 清除提示
            break;
        case QValidator::Intermediate:
            // 状态：不完整。用黄色边框提示。
            lineEdit->setStyleSheet("border: 1px solid orange;");
            lineEdit->setToolTip(tr("input not compelete"));
            break;
        case QValidator::Invalid:
            // 状态：非法。用红色边框警告。
            lineEdit->setStyleSheet("border: 1px solid red;");
            lineEdit->setToolTip(tr("input illegal character"));
            break;
    }
}

// 从元数据更新组件
void MapConfig::update_components_frommeta() {
    auto &basemeta = map_ref->base_metadata();
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

void MapConfig::update_ifcompeleted_dialog() {
    bool compeleted{true};
    this->dialog_compeleted = !ui->audio_path_lineedit->text().isEmpty() &&
                              !ui->title_ascii_lineedit->text().isEmpty() &&
                              !ui->artist_ascii_lineedit->text().isEmpty() &&
                              !ui->preferencebpm_lineedit->text().isEmpty() &&
                              !ui->trackcount_lineedit->text().isEmpty() &&
                              !ui->map_length_lineedit->text().isEmpty() &&
                              !ui->cover_lineedit->text().isEmpty() &&
                              !ui->author_lineedit->text().isEmpty() &&
                              !ui->version_lineedit->text().isEmpty();
    emit validityChanged(this->dialog_compeleted);
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
        // 设置选中项
        if (auto track =
                trackmanager
                    ->get_track(ui->audio_track_selectbox->currentText())
                    .lock()) {
            selected_track = track;
        }
    }
}

// 检查音频轨道位置
int MapConfig::find_track_indexinCombobox() {
    return ui->audio_track_selectbox->findText(QString::fromStdString(
        map_ref->base_metadata().main_audio_path.generic_string()));
}
