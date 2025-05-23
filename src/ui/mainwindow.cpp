#include "mainwindow.h"

#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include <QVBoxLayout>
#include <memory>
#include <string>

#include "../audio/BackgroundAudio.h"
#include "./ui_mainwindow.h"
#include "controllers/timecontroller.h"
#include "mmetas.h"
#include "objectinfoui.h"
#include "pageui/editorui/CanvasContainer.h"
#include "pageui/editorui/meditorarea.h"
#include "pageui/mpage.h"
#include "src/util/mutil.h"
#include "timinginfoui.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    auto infoui = ui->infomation_widget;

    page = ui->page_widget;

    connect(XLogger::uilogger, &MUiLogger::info, [=](const QString& message) {
        infoui->log(MLogLevel::MLOGINFO, message);
    });
    connect(XLogger::uilogger, &MUiLogger::warn, [=](const QString& message) {
        infoui->log(MLogLevel::MLOGWARN, message);
    });
    connect(XLogger::uilogger, &MUiLogger::error, [=](const QString& message) {
        infoui->log(MLogLevel::MLOGERROR, message);
    });

    BackgroundAudio::init();

    // 注册QVariant数据类型
    // 注册 string 类型
    qRegisterMetaType<std::string>("std::string");
    // 注册 shared_ptr<MMap> 类型
    qRegisterMetaType<std::shared_ptr<MMap>>("std::shared_ptr<MMap>");
    // 注册 shared_ptr<MapWorkProject> 类型
    qRegisterMetaType<std::shared_ptr<MapWorkProject>>(
        "std::shared_ptr<MapWorkProject>");

    init_actions();

    // 连接文件浏览器和项目管理器信号
    connect(ui->file_controller, &FileBrowserController::open_project,
            ui->project_controller, &MProjectController::new_project);

    // 连接项目选择map信号
    connect(ui->project_controller, &MProjectController::select_map,
            ui->page_widget, &MPage::project_controller_select_map);

    // 连接窗口标题更新信号
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &GLCanvas::update_window_title_suffix, this,
            &MainWindow::update_window_title);

    // 连接选择物件槽
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::select_object,
            ui->infomation_widget->mmetas->objinfo_ref,
            &ObjectInfoui::on_canvasSelectObject);

    // 连接选择timing槽
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::select_timing,
            ui->infomation_widget->mmetas->timinginfo_ref,
            &TimingInfoui::on_canvasSelectTimings);

    // 连接调节时间线信号槽
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::timeline_zoom_adjusted,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_canvasAdjustTimelineZoom);
    // 连接调节时间信号槽
    connect(ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::time_edited,
            ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::on_timeedit_setpos);

    // 连接时间控制器选择map槽
    connect(ui->page_widget->edit_area_widget, &MEditorArea::switched_map,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_selectnewmap);

    // 连接画布信号和时间控制器暂停槽(来回)
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::pause_signal,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_canvasPause);
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::pause_signal,
            ui->page_widget->edit_area_widget, &MEditorArea::on_canvasPause);
    connect(ui->page_widget->edit_area_widget,
            &MEditorArea::pause_button_changed,
            ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::on_timecontroller_pause_button_changed);

    // 连接画布槽和时间控制器信号
    // 时间控制器暂停->画布暂停响应
    connect(ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_canvasPause,
            ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::on_timecontroller_pause_button_changed);

    // 时间控制器变速->画布变速响应
    connect(ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::playspeed_changed,
            ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::on_timecontroller_speed_changed);
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::current_time_stamp_changed,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::oncanvas_timestampChanged);

    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::current_time_stamp_changed,
            ui->page_widget->edit_area_widget,
            &MEditorArea::oncanvas_timestampChanged);

    // 连接bpm,时间线速度变化槽
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::current_absbpm_changed,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_currentBpmChanged);
    connect(ui->page_widget->edit_area_widget->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::current_timeline_speed_changed,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_currentTimelineSpeedChanged);

    // 时间控制器-进度条
    connect(ui->page_widget->edit_area_widget,
            &MEditorArea::progress_pos_changed,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::oncanvas_timestampChanged);

    connect(ui->page_widget->edit_area_widget,
            &MEditorArea::pause_button_changed,
            ui->infomation_widget->mmetas->time_audio_controller_ref,
            &TimeController::on_canvasPause);
}

MainWindow::~MainWindow() { delete ui; }

// 更新窗口标题
void MainWindow::update_window_title(QString& suffix) {
    setWindowTitle(tr("Music Map Maker -") + suffix);
}

// 使用主题
void MainWindow::use_theme(GlobalTheme theme) {
    // TODO(xiang 2025-04-16): 实现多主题切换
    current_theme = theme;
    QColor file_button_color;
    switch (theme) {
        case GlobalTheme::DARK: {
            file_button_color = QColor(255, 255, 255);
            QFile file(":/QtThemeDark/theme/Flat/Dark/Pink/Orange.qss");
            file.open(QFile::ReadOnly);
            setStyleSheet(file.readAll());
            ui->actionDark->setChecked(true);

            break;
        }
        case GlobalTheme::LIGHT: {
            file_button_color = QColor(0, 0, 0);
            QFile file(":/QtThemeLight/theme/Flat/Light/Brown/DeepOrange.qss");
            file.open(QFile::ReadOnly);
            setStyleSheet(file.readAll());
            ui->actionLight->setChecked(true);
            break;
        }
    }

    // 设置文件操作图标颜色
    mutil::set_action_svgcolor(ui->actionOpen, ":/icons/file.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionOpen_Directory,
                               ":/icons/folder-open.svg", file_button_color);

    mutil::set_action_svgcolor(ui->actionSave, ":/icons/file-alt.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionSave_As, ":/icons/file-export.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionPack, ":/icons/file-archive.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionPack_As, ":/icons/file-archive.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionNew_Version, ":/icons/plus.svg",
                               file_button_color);
    mutil::set_menu_svgcolor(ui->menuSwitch_Version, ":/icons/exchange-alt.svg",
                             file_button_color);

    // 设置编辑操作图标颜色
    mutil::set_action_svgcolor(ui->actionUndo, ":/icons/undo-alt.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionRedo, ":/icons/redo-alt.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionYank, ":/icons/copy.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionCut, ":/icons/cut.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionPaste, ":/icons/paste.svg",
                               file_button_color);
    mutil::set_action_svgcolor(ui->actionDelete, ":/icons/trash.svg",
                               file_button_color);

    // 设置文件浏览器图标颜色
    ui->file_controller->use_theme(theme);

    // 设置page主题
    ui->page_widget->use_theme(theme);

    // 设置info主题
    ui->infomation_widget->use_theme(theme);
}

// 初始化actions的信号连接
void MainWindow::init_actions() {
    connect(ui->actionSave, &QAction::triggered,
            ui->page_widget->edit_area_widget, &MEditorArea::on_saveMap);
    connect(ui->actionSave_As, &QAction::triggered,
            ui->page_widget->edit_area_widget, &MEditorArea::on_saveMapAs);
    // 选择主题
    connect(ui->actionDark, &QAction::triggered, [&]() {
        ui->actionDark->setChecked(true);
        ui->actionLight->setChecked(false);
        use_theme(GlobalTheme::DARK);
    });
    connect(ui->actionLight, &QAction::triggered, [&]() {
        ui->actionDark->setChecked(false);
        ui->actionLight->setChecked(true);
        use_theme(GlobalTheme::LIGHT);
    });
}
