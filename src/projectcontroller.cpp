#include "projectcontroller.h"

#include <qlogging.h>
#include <qnamespace.h>
#include <qsharedpointer.h>
#include <qtmetamacros.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QUrl>
#include <algorithm>
#include <filesystem>
#include <memory>

#include "mmm/MapWorkProject.h"
#include "ui_mprojectcontroller.h"

MProjectController::MProjectController(QWidget* parent)
    : QWidget(parent), ui(new Ui::MProjectController) {
  ui->setupUi(this);

  // 初始化最后一次选择的为家目录
  last_select_directory = QDir::homePath();

  // 初始化listview模型
  QStandardItemModel* map_list_model =
      new QStandardItemModel(ui->map_list_view);
  QStandardItemModel* audio_list_model =
      new QStandardItemModel(ui->audio_list_view);
  QStandardItemModel* image_list_model =
      new QStandardItemModel(ui->image_list_view);
  QStandardItemModel* video_list_model =
      new QStandardItemModel(ui->video_list_view);

  // 应用并设置不可编辑
  ui->map_list_view->setModel(map_list_model);
  ui->map_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->audio_list_view->setModel(audio_list_model);
  ui->audio_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->image_list_view->setModel(image_list_model);
  ui->image_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->video_list_view->setModel(video_list_model);
  ui->video_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // 默认无选择
  ui->project_selector->addItem(QString(tr("No Project")));
}

MProjectController::~MProjectController() { delete ui; }

// 新项目槽函数
void MProjectController::new_project(std::shared_ptr<MapWorkProject>& project) {
  // 添加到项目列表
  project_list.emplace_back(project);

  // 添加选中项
  ui->project_selector->addItem(
      QString::fromStdString(project->config.project_name),
      QVariant::fromValue(project));

  // 立即选中此项目
  ui->project_selector->setCurrentIndex(ui->project_selector->count() - 1);

  // 检查关闭项目按钮是否启用
  if (!ui->close_project_button->isEnabled()) {
    ui->close_project_button->setEnabled(true);
  }
}

// 选择项目事件
void MProjectController::on_project_selector_currentIndexChanged(int index) {
  // 获取当前选中项的数据
  QVariant var = ui->project_selector->currentData();
  auto project = var.value<std::shared_ptr<MapWorkProject>>();
  select_project(project);
}

// 选择项目
void MProjectController::select_project(
    std::shared_ptr<MapWorkProject>& project) {
  auto map_model =
      qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
  map_model->clear();
  auto audio_model =
      qobject_cast<QStandardItemModel*>(ui->audio_list_view->model());
  audio_model->clear();
  auto image_model =
      qobject_cast<QStandardItemModel*>(ui->image_list_view->model());
  image_model->clear();
  auto video_model =
      qobject_cast<QStandardItemModel*>(ui->video_list_view->model());
  video_model->clear();
  if (project) {
    // 初始化每个tabwidget显示内容
    // maps
    for (const auto& map : project->maps) {
      QStandardItem* map_item =
          new QStandardItem(QString::fromStdString(map->map_name));
      map_item->setData(QVariant::fromValue(map), Qt::UserRole + 1);
      map_model->appendRow(map_item);
    }

    // audios
    for (const auto& audio_path : project->audio_paths) {
      std::filesystem::path p(audio_path);
      QStandardItem* audio_path_item =
          new QStandardItem(QString::fromStdString(p.filename().string()));
      audio_path_item->setData(QVariant::fromValue(audio_path),
                               Qt::UserRole + 1);
      audio_model->appendRow(audio_path_item);
    }

    // images
    for (const auto& image_path : project->image_paths) {
      std::filesystem::path p(image_path);
      QStandardItem* image_path_item =
          new QStandardItem(QString::fromStdString(p.filename().string()));
      image_path_item->setData(QVariant::fromValue(image_path),
                               Qt::UserRole + 1);
      image_model->appendRow(image_path_item);
    }

    // videos
    for (const auto& video_path : project->video_paths) {
      std::filesystem::path p(video_path);
      QStandardItem* video_path_item =
          new QStandardItem(QString::fromStdString(p.filename().string()));
      video_path_item->setData(QVariant::fromValue(video_path),
                               Qt::UserRole + 1);
      video_model->appendRow(video_path_item);
    }
  }
}

// 谱面列表双击事件
void MProjectController::on_map_list_view_doubleClicked(
    const QModelIndex& index) {
  auto map_model =
      qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
  auto map = map_model->itemFromIndex(index)
                 ->data(Qt::UserRole + 1)
                 .value<std::shared_ptr<MMap>>();

  // 发送选择map信号
  emit select_map(map);
}

// 音频列表双击事件
void MProjectController::on_audio_list_view_doubleClicked(
    const QModelIndex& index) {}

// 图片列表双击事件
void MProjectController::on_image_list_view_doubleClicked(
    const QModelIndex& index) {}

// 视频列表双击事件
void MProjectController::on_video_list_view_doubleClicked(
    const QModelIndex& index) {}

// 谱面列表上下文菜单事件
void MProjectController::on_map_list_view_customContextMenuRequested(
    const QPoint& pos) {
  // 使用当前选中项而不是点击位置
  QModelIndex index = ui->map_list_view->currentIndex();

  // 生成菜单
  QMenu menu;

  // 根据点击位置添加菜单项
  if (!index.isValid()) {
  } else {
    // 获得选中的map
    auto selected_map =
        qobject_cast<QStandardItemModel*>(ui->map_list_view->model())
            ->itemFromIndex(index)
            ->data()
            .value<std::shared_ptr<MMap>>();
    // 在文件管理器中打开
    menu.addAction(tr("Open In FileBrowser"),
                   [
                       // 复制捕获防止智能指针引用丢失
                       = ]() {
                     // qDebug() << selected_map->map_name;
                     // 获取文件上一级路径
                     QDir dir(selected_map->map_file_path.parent_path());
                     // 转换为本地文件URL
                     QUrl url = QUrl::fromLocalFile(dir.absolutePath());
                     // 使用文件管理器打开
                     QDesktopServices::openUrl(url);
                   });
  }
  // 添加通用菜单项
  menu.addSeparator();
  menu.addAction(tr("Import Map"), [&]() {
    auto options = QFileDialog::DontUseNativeDialog;
    auto fileNames = QFileDialog::getOpenFileNames(
        this, tr("选择谱面"), last_select_directory,
        tr("谱面文件(*.osu *.imd *.mc)"), nullptr, options);

    // TODO(xiang 2025-04-17): 实现项目中导入谱面
    for (auto& name : fileNames) {
      qDebug() << "selected:" << name;
    }
  });
  menu.addAction(tr("Create New Map"), [&]() {
    // 调用创建谱面函数
    qDebug() << "创建谱面";
  });

  // 显示菜单
  menu.exec(ui->map_list_view->viewport()->mapToGlobal(pos));
}

// 音频列表上下文菜单事件
void MProjectController::on_audio_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 图片列表上下文菜单事件
void MProjectController::on_image_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 视频列表上下文菜单事件
void MProjectController::on_video_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 关闭项目事件
void MProjectController::on_close_project_button_clicked() {
  auto current_select_text = ui->project_selector->currentText();
  if (current_select_text != "No Project") {
    // 获取选中的项目指针
    QVariant var = ui->project_selector->currentData();
    auto project = var.value<std::shared_ptr<MapWorkProject>>();
    auto project_it =
        std::find(project_list.begin(), project_list.end(), project);
    if (project_it != project_list.end()) {
      // 移除当前选中项
      ui->project_selector->removeItem(ui->project_selector->currentIndex());
      // 从项目列表移除
      project_list.erase(project_it);
      // 检查当前是否还有其他项目
      if (!project_list.empty()) {
        // 选中另一个项目
        on_project_selector_currentIndexChanged(1);
      } else {
        // 选中空白并禁用关闭项目按钮
        ui->project_selector->setCurrentIndex(0);
        std::shared_ptr<MapWorkProject> p = nullptr;
        select_project(p);
        ui->close_project_button->setEnabled(false);
      }
    }
  }
}
