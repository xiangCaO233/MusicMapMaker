#include <audio/track/trackmanager.h>
#include <mainwindow.h>
#include <project/projectmanager.h>
#include <qobjectdefs.h>
#include <ui_mainwindow.h>

#include <QFile>
#include <canvas/map/MapCanvas.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/project/MProject.hpp>

// 全局样式表
QString MainWindow::global_style_sheet;

// 全部设置
Settings MainWindow::settings{};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    qRegisterMetaType<std::string>("std::string");
    // 注册 MMap* 类型
    qRegisterMetaType<MMap*>("MMap*");

    // 初始化项目服务
    ui->project_manager->initService(ui->editor->canvas(), ui->track_manager);

    // 捕获ui指针
    auto capui = ui;

    auto canvas = ui->editor->canvas();
    connect(canvas, &GLCanvas::update_window_suffix, this,
            &MainWindow::update_title_suffix);

    connect(ui->track_manager, &TrackManager::close,
            [capui]() { capui->actionTrack_Manager->setChecked(false); });

    connect(ui->project_manager, &ProjectManager::close,
            [capui]() { capui->actionProject_Manager->setChecked(false); });

    connect(canvas, &MapCanvas::skinInitialized, this,
            &MainWindow::onDefSkinInitialized);

    connect(ui->track_manager, &TrackManager::audioLoadcbk_initialized, canvas,
            &MapCanvas::onAudioLoadcbkInitialized);

    initActions();
}

MainWindow::~MainWindow() {
    // 先清理项目(需要使用editor的gl上下文移除纹理)
    delete ui;
}
// 使用主题
void MainWindow::use_theme(GlobalTheme theme) {
    // current_theme = theme;
    // settings.global_theme = theme;
    QColor button_icon_color;
    switch (theme) {
        case GlobalTheme::OPEN_DARK: {
            button_icon_color = QColor(255, 255, 255);
            QFile file(":/QtThemeDark/theme/Flat/Dark/Pink/Orange.qss");
            file.open(QFile::ReadOnly);
            global_style_sheet = file.readAll();
            setStyleSheet(global_style_sheet);
            // ui->actionDark->setChecked(true);
            break;
        }
        case GlobalTheme::OPEN_LIGHT: {
            button_icon_color = QColor(0, 0, 0);
            QFile file(":/QtThemeLight/theme/Flat/Light/Brown/DeepOrange.qss");
            file.open(QFile::ReadOnly);
            global_style_sheet = file.readAll();
            setStyleSheet(global_style_sheet);
            // ui->actionLight->setChecked(true);
            break;
        }
        case GlobalTheme::COLIN_DARK: {
            button_icon_color = QColor(255, 255, 255);
            QFile file(":/qdarkstyle/dark/darkstyle.qss");
            file.open(QFile::ReadOnly);
            global_style_sheet = file.readAll();
            setStyleSheet(global_style_sheet);
            // ui->actionDark->setChecked(true);
            break;
        }
        case GlobalTheme::COLIN_LIGHT: {
            button_icon_color = QColor(0, 0, 0);
            QFile file(":/qdarkstyle/light/lightstyle.qss");
            file.open(QFile::ReadOnly);
            global_style_sheet = file.readAll();
            setStyleSheet(global_style_sheet);
            // ui->actionLight->setChecked(true);
            break;
        }
    }
}

// 更新标题后缀
void MainWindow::update_title_suffix(const QString& suffix) {
    setWindowTitle(tr("MusicMapMaker-->") + suffix);
}

// 默认皮肤初始化完成
void MainWindow::onDefSkinInitialized() {
    // 触发音效加载回调
    emit ui->track_manager->audioLoadcbk_initialized(ui->track_manager);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    ui->track_manager->close();
    ui->project_manager->close();
}
