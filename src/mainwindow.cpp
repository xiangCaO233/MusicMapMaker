#include "mainwindow.h"

#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>

#include <QFileSystemModel>

#include "./ui_mainwindow.h"
#include "AudioManager.h"
#include "mmm/map/osu/OsuMap.h"
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

  // 获取家目录
  QString home = QDir::homePath();

  // 设置地址栏为家目录
  ui->address_line->setText(home);

  // 初始化文件浏览器
  // 文件管理器模型
  QFileSystemModel* file_model = new QFileSystemModel;
  file_model->setRootPath(home);
  // 文件管理器过滤器(不显示.显示..)
  file_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

  // 使用文件管理器模型
  ui->file_browser_treeview->setModel(file_model);

  // 隐藏修改日期
  ui->file_browser_treeview->hideColumn(3);
  // 隐藏类型
  ui->file_browser_treeview->hideColumn(2);

  // 跳转到当前运行目录
  ui->file_browser_treeview->setRootIndex(file_model->index(home));
  ui->file_browser_treeview->setColumnWidth(0, 400);
  ui->file_browser_treeview->setColumnWidth(1, 80);
  // 初始化文件浏览器管理器
  filebrowercontroller = std::make_shared<MFileBrowserController>();

  // 获得画布指针
  canvas = ui->canvas;

  // 传递引用
  canvas->audio_manager = audio_manager;

  // 默认使用Dark主题
  use_theme(GlobalTheme::DARK);

  // 连接文件浏览器和项目管理器信号
  connect(filebrowercontroller.get(), &MFileBrowserController::open_project,
          ui->project_controller, &MProjectController::new_project);

  // 连接项目选择map信号
  connect(ui->project_controller, &MProjectController::select_map, this,
          &MainWindow::project_controller_select_map);

  // 设置map
  // auto omap6k1 = std::make_shared<OsuMap>();
  // omap6k1->load_from_file(
  //    "../resources/map/Designant - Designant/Designant - Designant. (Benson_)
  //    "
  //    "[Designant].osu");
  // auto omap4k1 = std::make_shared<OsuMap>();
  // omap4k1->load_from_file(
  //    "../resources/map/Lia - Poetry of Birds/Lia - Poetry of Birds "
  //    "(xiang_233) [full version].osu");
  // auto omap4k2 = std::make_shared<OsuMap>();
  // omap4k2->load_from_file(
  //    "../resources/map/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku - "
  //    "shojo rei/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku - shojo rei
  //    "
  //    "(xiang_233) [(LN)NM lv.29].osu");
  // canvas->switch_map(omap6k1);
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

  // 设置文件管理器按钮图标颜色
  mutil::set_button_svgcolor(ui->cdup, ":/icons/angle-left.svg",
                             file_button_color);
  mutil::set_button_svgcolor(ui->cdnext, ":/icons/angle-right.svg",
                             file_button_color);
  mutil::set_button_svgcolor(ui->address_confirm, ":/icons/arrow-right.svg",
                             file_button_color);
  mutil::set_button_svgcolor(ui->search, ":/icons/search.svg",
                             file_button_color);
}

// 选择音频输出设备事件
void MainWindow::on_audio_device_selector_currentIndexChanged(int index) {
  // 获取并设置当前选中的音频设备
  auto data = ui->audio_device_selector->currentData();
  current_use_device = data.value<std::shared_ptr<XOutputDevice>>();
}

// 文件浏览器上下文菜单
void MainWindow::on_file_browser_treeview_customContextMenuRequested(
    const QPoint& pos) {
  QModelIndex index = ui->file_browser_treeview->indexAt(pos);
  if (!index.isValid()) return;
  auto click_abs_path = index.data(Qt::UserRole + 1).toString();
  auto click_item = QFileInfo(click_abs_path);

  // 生成菜单
  QMenu menu;

  // 根据项目类型添加不同的菜单项
  if (click_item.isDir()) {
    // 文件夹
    // qDebug() << "click folder: " << click_abs_path;
    menu.addAction(tr("Open Folder"), [&]() {
      filebrowercontroller->on_open_folder(
          qobject_cast<QFileSystemModel*>(ui->file_browser_treeview->model()),
          index);
    });
    menu.addAction(tr("Open As Project"), [&]() {
      filebrowercontroller->on_open_folder_as_project(click_abs_path);
    });
    menu.addAction(tr("New File"), [&]() {
      filebrowercontroller->on_new_file_infolder(click_abs_path);
    });
  } else {
    // 文件
    // qDebug() << "click file: " << click_abs_path;
    menu.addAction(tr("Open File"), [&]() {
      filebrowercontroller->on_open_file(click_abs_path);
    });
    menu.addAction(tr("Rename"), [&]() {
      filebrowercontroller->on_rename_file(click_abs_path);
    });
    menu.addAction(tr("Delete"), [&]() {
      filebrowercontroller->on_delete_file(click_abs_path);
    });
  }

  // 添加通用菜单项
  menu.addSeparator();
  menu.addAction(tr("Properties"), [&]() {
    filebrowercontroller->on_show_properties(click_abs_path);
  });

  // 显示菜单
  menu.exec(ui->file_browser_treeview->viewport()->mapToGlobal(pos));
}

// 项目控制器选择了map事件
void MainWindow::project_controller_select_map(std::shared_ptr<MMap>& map) {
  // 切换画布到选择的map
  ui->canvas->switch_map(map);
}
