#include "mfilebrowsercontroller.h"

#include <qdir.h>
#include <qfileinfo.h>

#include <QFileSystemModel>

#include "log/colorful-log.h"
#include "mainwindow.h"
#include "ui_mfilebrowsercontroller.h"
#include "util/mutil.h"

FileBrowserController::FileBrowserController(QWidget* parent)
    : QWidget(parent), ui(new Ui::FileBrowserController) {
  ui->setupUi(this);
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

  // 跳转到目录
  ui->file_browser_treeview->setRootIndex(file_model->index(home));
  ui->file_browser_treeview->setColumnWidth(0, 400);
  ui->file_browser_treeview->setColumnWidth(1, 80);

  // 禁用展开
  // ui->file_browser_treeview->setItemsExpandable(false);
  // ui->file_browser_treeview->setRootIsDecorated(false);
}

FileBrowserController::~FileBrowserController() { delete ui; }

// 使用主题
void FileBrowserController::use_theme(GlobalTheme theme) {
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
  // 设置文件管理器按钮图标颜色
  mutil::set_button_svgcolor(ui->cdup, ":/icons/angle-left.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->cdnext, ":/icons/angle-right.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->address_confirm, ":/icons/arrow-right.svg",
                             file_button_color, 28, 28);
  mutil::set_button_svgcolor(ui->search, ":/icons/search.svg",
                             file_button_color, 28, 28);
}

// TODO(xiang 2025-04-16): 实现文件管理器功能
// 文件浏览器上下文菜单
void MainWindow::on_file_browser_treeview_customContextMenuRequested(
    const QPoint& pos) {}

void FileBrowserController::on_file_browser_treeview_customContextMenuRequested(
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
      // filebrowercontroller->on_menu_open_folder(
      //     qobject_cast<QFileSystemModel*>(ui->file_browser_treeview->model()),
      //     index);
    });
    menu.addAction(tr("Open As Project"), [&]() {
      // TODO(xiang 2025-04-16): 实现打开工程
      std::filesystem::path ppath(click_abs_path.toStdString());
      XINFO("打开项目目录:[" + ppath.string() + "]");
      auto project = std::make_shared<MapWorkProject>(ppath);
      // 发送信号
      emit open_project(project);
    });
    menu.addAction(tr("New File"), [&]() {
      // filebrowercontroller->on_menu_new_file_infolder(click_abs_path);
    });
  } else {
    // 文件
    menu.addAction(tr("Open File"), [&]() {
      // filebrowercontroller->on_menu_open_file(click_abs_path);
    });
    menu.addAction(tr("Rename"), [&]() {
      // filebrowercontroller->on_menu_rename_file(click_abs_path);
    });
    menu.addAction(tr("Delete"), [&]() {
      // filebrowercontroller->on_menu_delete_file(click_abs_path);
    });
  }

  // 添加通用菜单项
  menu.addSeparator();
  menu.addAction(tr("Properties"), [&]() {
    // filebrowercontroller->on_menu_show_properties(click_abs_path);
  });

  // 显示菜单
  menu.exec(ui->file_browser_treeview->viewport()->mapToGlobal(pos));
}
