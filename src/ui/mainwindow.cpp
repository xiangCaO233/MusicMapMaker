#include <audio/track/trackmanager.h>
#include <mainwindow.h>
#include <project/projectmanager.h>
#include <qobjectdefs.h>
#include <timingmanager.h>
#include <ui_mainwindow.h>

#include <QFile>
#include <action/ActionManager.hpp>
#include <canvas/map/MapCanvas.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/project/MProject.hpp>

#include "colorful-log.h"

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
    ui->project_manager->initService(ui->editor->canvas());

    connectAll();

    initActions();

    ui->editor->bindToolActions();

    // 跟随系统主题应用默认主题
    XINFO("System theme lightness:" +
          std::to_string(QApplication::palette().window().color().lightness()));
    if (QApplication::palette().window().color().lightness() < 128) {
        use_theme(GlobalTheme::COLIN_DARK);
        ui->actionColin_Dark->setChecked(true);
    } else {
        use_theme(GlobalTheme::COLIN_LIGHT);
        ui->actionColin_Light->setChecked(true);
    }
}

MainWindow::~MainWindow() {
    // 先清理项目(需要使用editor的gl上下文移除纹理)
    delete ui;
}

// 初始化所有信号连接
void MainWindow::connectAll() {
    auto canvas = ui->editor->canvas();
    // 项目相关
    // 连接激活项目时的信号-项目配置dock的初始化
    auto project_setting_dock_content = ui->project_config;
    connect(ui->project_manager->get_service(),
            &ProjectService::activateProject, project_setting_dock_content,
            &ProjectConfig::bind_config);
    // 连接项目配置更新的信号-轨道布局
    connect(ui->project_config, &ProjectConfig::layout_changed, canvas,
            &MapCanvas::onLayoutUpdated);

    // 编辑相关
    // 连接激活map事件-同步timing编辑器列表
    connect(ui->project_manager->get_service(), &ProjectService::activateMap,
            ui->timing_editor, &TimingManager::onMapUpdated);
    // 初始化画布后为timing编辑器绑定编辑命令队列
    connect(canvas, &MapCanvas::toolcmdqInitialized, ui->timing_editor,
            &TimingManager::bind_toolcmdq);
    // 绑定默认分拍控制信号
    connect(ui->editor, &MapEditor::updateGeneratedDivisors, canvas,
            &MapCanvas::onUpdateDivs);

    // 绑定画布的渲染信号-显示帧率
    connect(canvas, &GLCanvas::update_window_suffix, this,
            &MainWindow::update_title_suffix);

    // timing编辑器的跳转到timing位置信号
    connect(ui->timing_editor, &TimingManager::navigateToTiming, canvas,
            &MapCanvas::gotoTiming);

    // 线程相关
    // 确保在数据循环结束后(所有工作线程停止)销毁项目
    connect(canvas, &GLCanvas::dataloop_stopped, ui->project_manager,
            &ProjectManager::onMapCanvasThreadStopped);

    // 确保在音频系统初始化后载入默认皮肤需要载入的所有音频
    connect(canvas, &MapCanvas::skinInitialized, ui->project_manager,
            &ProjectManager::onDefSkinInitialized);
}

// 使用主题
void MainWindow::use_theme(GlobalTheme theme) {
    current_theme = theme;
    settings.theme = theme;
    QColor button_icon_color;
    QFile file;
    switch (theme) {
        case GlobalTheme::OPEN_DARK: {
            button_icon_color = QColor(225, 225, 225);
            file.setFileName(":/QtThemeDark/theme/Flat/Dark/Pink/Orange.qss");
            break;
        }
        case GlobalTheme::OPEN_LIGHT: {
            button_icon_color = QColor(23, 23, 23);
            file.setFileName(
                ":/QtThemeLight/theme/Flat/Light/Brown/DeepOrange.qss");
            break;
        }
        case GlobalTheme::COLIN_DARK: {
            button_icon_color = QColor(225, 225, 225);
            file.setFileName(":/qdarkstyle/dark/darkstyle.qss");
            break;
        }
        case GlobalTheme::COLIN_LIGHT: {
            button_icon_color = QColor(23, 23, 23);
            file.setFileName(":/qdarkstyle/light/lightstyle.qss");
            break;
        }
    }
    if (file.open(QFile::ReadOnly)) {
        global_style_sheet = file.readAll();
        qApp->setStyleSheet(global_style_sheet);
        file.close();
    }
    ui->editor->use_theme(theme);

    // actions 的图标
    auto am = ActionManager::instance();

    mutil::set_action_svgcolor(am->getAction("canvas.switchhandtool"),
                               "://icons/hand-rock.svg", button_icon_color, 16,
                               16);
    mutil::set_action_svgcolor(am->getAction("canvas.switchnotetool"),
                               "://icons/edit.svg", button_icon_color, 16, 16);
}

// 更新标题后缀
void MainWindow::update_title_suffix(const QString& suffix) {
    setWindowTitle(tr("MusicMapMaker-->") + suffix);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    ui->editor->close();
    ui->project_manager->close();
    qApp->quit();
}
