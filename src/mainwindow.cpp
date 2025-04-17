#include "mainwindow.h"

#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>

#include <QFileSystemModel>

#include "./ui_mainwindow.h"
#include "AudioManager.h"
#include "MapWorkspaceCanvas.h"
#include "hellouserpage.h"
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

  // 默认使用Dark主题
  use_theme(GlobalTheme::DARK);

  // 添加欢迎页
  hello_page = new HelloUserPage(ui->main_tabs_widget);

  // 根据设置判断是否添加欢迎页
  if (settings.show_hello_page) {
    ui->main_tabs_widget->addTab(hello_page, tr("Welcom Use MMM"));
    // 获取当前所有子组件的大小
    QList<int> sizes = ui->right_splitter->sizes();
    // 修改tab页组件的宽度
    sizes[0] = 480;
    // 应用新的大小
    ui->right_splitter->setSizes(sizes);
  } else {
    hello_page->hide();
  }
  // 设置Tabs可关闭
  ui->main_tabs_widget->setTabsClosable(true);

  // 连接关闭信号
  connect(ui->main_tabs_widget, &QTabWidget::tabCloseRequested,
          [this](int index) {
            auto tabindexit = tabindex_maps_map.end();
            if ((tabindexit = tabindex_maps_map.find(index)) ==
                tabindex_maps_map.end()) {
              // 非画布标签页直接关闭
              ui->main_tabs_widget->removeTab(index);
            } else {
              // 处理画布标签页的关闭
            }
          });

  // 连接文件浏览器和项目管理器信号
  connect(filebrowercontroller.get(), &MFileBrowserController::open_project,
          ui->project_controller, &MProjectController::new_project);

  // 连接项目选择map信号
  connect(ui->project_controller, &MProjectController::select_map, this,
          &MainWindow::project_controller_select_map);
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

  // 设置文件管理器按钮图标颜色
  mutil::set_button_svgcolor(ui->cdup, ":/icons/angle-left.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->cdnext, ":/icons/angle-right.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->address_confirm, ":/icons/arrow-right.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->search, ":/icons/search.svg",
                             file_button_color, 28, 28);

  // 设置工具栏按钮图标颜色
  mutil::set_toolbutton_svgcolor(ui->default_divisor_policy_toolbutton,
                                 ":/icons/stream.svg", file_button_color, 16,
                                 16);
  mutil::set_toolbutton_svgcolor(ui->bookmark_toolbutton,
                                 ":/icons/bookmark.svg", file_button_color, 16,
                                 16);
  mutil::set_button_svgcolor(ui->audio_controller_button,
                             ":/icons/sliders-h.svg", file_button_color, 16,
                             16);
  mutil::set_button_svgcolor(ui->fit_size_to_orbitcount_button,
                             ":/icons/expand.svg", file_button_color, 16, 16);
  mutil::set_button_svgcolor(ui->magnet_todivisor_button, ":/icons/magnet.svg",
                             file_button_color, 16, 16);

  // 两个状态
  mutil::set_button_svgcolor(ui->wheel_direction_button,
                             ":/icons/long-arrow-alt-up.svg", file_button_color,
                             16, 16);
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

// 滚动方向切换按钮触发
void MainWindow::on_wheel_direction_button_toggled(bool checked) {
  use_natural_wheel = checked;
  // 根据主题切换图标颜色
  QColor file_button_color;
  switch (current_theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
      break;
    }
  }

  // 两个状态
  mutil::set_button_svgcolor(ui->wheel_direction_button,
                             (checked ? ":/icons/long-arrow-alt-down.svg"
                                      : ":/icons/long-arrow-alt-up.svg"),
                             file_button_color, 16, 16);
}

// 交换标签页内容
void safeSwapTabs(QTabWidget* tabWidget, int i, int j) {
  if (i == j) return;

  // 阻塞信号避免不必要的刷新
  const bool wasBlocked = tabWidget->blockSignals(true);

  // 获取widget前先保存状态
  int current = tabWidget->currentIndex();
  QWidget* w1 = tabWidget->widget(i);
  QWidget* w2 = tabWidget->widget(j);

  // 临时解除父子关系
  w1->setParent(nullptr);
  w2->setParent(nullptr);

  // 移除并重新插入
  tabWidget->removeTab(qMax(i, j));
  tabWidget->removeTab(qMin(i, j));

  tabWidget->insertTab(i, w2, tabWidget->tabText(j));
  tabWidget->insertTab(j, w1, tabWidget->tabText(i));

  // 恢复当前索引
  if (current == i) {
    tabWidget->setCurrentIndex(j);
  } else if (current == j) {
    tabWidget->setCurrentIndex(i);
  }

  // 恢复信号
  tabWidget->blockSignals(wasBlocked);
}

// 项目控制器选择了map事件
void MainWindow::project_controller_select_map(std::shared_ptr<MMap>& map) {
  // 查找是否存在已打开的此map的标签页
  auto tabindex_it = maps_tabindex_map.find(map);

  MapWorkspaceCanvas* current_canvas;
  if (tabindex_it == maps_tabindex_map.end()) {
    // 无已打开的谱面标签页
    tabindex_it = maps_tabindex_map.try_emplace(map).first;

    bool share{false};
    if (!canvas_context) {
      // 初始化上下文
      canvas_context = new MapWorkspaceCanvas(ui->main_tabs_widget);
      current_canvas = canvas_context;
    } else {
      // 新建并共享gl上下文
      current_canvas = new MapWorkspaceCanvas(ui->main_tabs_widget);
      share = true;
    }

    // 传递音频管理器
    current_canvas->audio_manager = audio_manager;
    // 新建画布上下文构造一个tab
    tabindex_it->second = ui->main_tabs_widget->addTab(
        current_canvas, QString::fromStdString(map->map_name));

    // 切换到这个标签
    // 先显示
    ui->main_tabs_widget->setCurrentIndex(tabindex_it->second);

    if (share) {
      current_canvas->context()->setShareContext(canvas_context->context());
    }

    // 切换画布map到选择的map
    current_canvas->switch_map(map);

    // 创建另一个映射
    tabindex_maps_map.try_emplace(tabindex_it->second, map);
  } else {
    // 直接切换到这个标签
    // 切换标签页
    ui->main_tabs_widget->setCurrentIndex(tabindex_it->second);
  }
}

// 切换当前标签页事件
void MainWindow::on_main_tabs_widget_currentChanged(int index) {
  // 更新最后选择的标签索引
  last_main_tab_index = index;
}
