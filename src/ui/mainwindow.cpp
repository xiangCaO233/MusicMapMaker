#include "mainwindow.h"

#include <qlogging.h>
#include <qobject.h>

#include <QVBoxLayout>
#include <memory>
#include <string>

#include "./ui_mainwindow.h"
#include "AudioManager.h"
#include "src/util/mutil.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  // 注册QVariant数据类型
  // 注册 string 类型
  qRegisterMetaType<std::string>("std::string");
  // 注册 shared_ptr<XOutputDevice> 类型
  qRegisterMetaType<std::shared_ptr<XOutputDevice>>(
      "std::shared_ptr<XOutputDevice>");
  // 注册 shared_ptr<MMap> 类型
  qRegisterMetaType<std::shared_ptr<MMap>>("std::shared_ptr<MMap>");
  // 注册 shared_ptr<MapWorkProject> 类型
  qRegisterMetaType<std::shared_ptr<MapWorkProject>>(
      "std::shared_ptr<MapWorkProject>");

  // 初始化音频管理器
  audio_manager = XAudioManager::newmanager();

  // 更新设备列表
  auto devices = audio_manager->get_outdevices();
  for (const auto& [device_id, device] : *devices) {
    ui->audio_device_selector->addItem(
        QString::fromStdString(device->device_name),
        QVariant::fromValue(device));
  }

  // 默认使用Dark主题
  use_theme(GlobalTheme::DARK);

  // 连接文件浏览器和项目管理器信号
  connect(ui->file_controller, &FileBrowserController::open_project,
          ui->project_controller, &MProjectController::new_project);

  // 连接项目选择map信号
  connect(ui->project_controller, &MProjectController::select_map,
          ui->page_widget, &MPage::project_controller_select_map);
}

MainWindow::~MainWindow() { delete ui; }

// 使用主题
void MainWindow::use_theme(GlobalTheme theme) {
  // TODO(xiang 2025-04-16): 实现多主题切换
  current_theme = theme;
  QColor file_button_color;
  switch (theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
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
}

// 选择音频输出设备事件
void MainWindow::on_audio_device_selector_currentIndexChanged(int index) {
  // 获取并设置当前选中的音频设备
  auto data = ui->audio_device_selector->currentData();
  current_use_device = data.value<std::shared_ptr<XOutputDevice>>();
}
