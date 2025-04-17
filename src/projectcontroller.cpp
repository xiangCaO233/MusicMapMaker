#include "projectcontroller.h"

#include <qlogging.h>
#include <qnamespace.h>
#include <qsharedpointer.h>
#include <qtmetamacros.h>

#include <QStandardItemModel>
#include <filesystem>
#include <memory>

#include "colorful-log.h"
#include "mmm/MapWorkProject.h"
#include "ui_mprojectcontroller.h"

MProjectController::MProjectController(QWidget* parent)
    : QWidget(parent), ui(new Ui::MProjectController) {
  ui->setupUi(this);

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
}

MProjectController::~MProjectController() { delete ui; }

// 新项目槽函数
void MProjectController::new_project(std::shared_ptr<MapWorkProject>& project) {
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
  XINFO("选择打开项目[" + project->config.project_name + "]");

  // 初始化每个tabwidget显示内容
  // maps
  auto map_model =
      qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
  map_model->clear();
  for (const auto& map : project->maps) {
    QStandardItem* map_item =
        new QStandardItem(QString::fromStdString(map->map_name));
    map_item->setData(QVariant::fromValue(map), Qt::UserRole);
    map_model->appendRow(map_item);
  }

  // audios
  auto audio_model =
      qobject_cast<QStandardItemModel*>(ui->audio_list_view->model());
  audio_model->clear();
  for (const auto& audio_path : project->audio_paths) {
    std::filesystem::path p(audio_path);
    QStandardItem* audio_path_item =
        new QStandardItem(QString::fromStdString(p.filename().string()));
    audio_path_item->setData(QVariant::fromValue(audio_path), Qt::UserRole);
    audio_model->appendRow(audio_path_item);
  }

  // images
  auto image_model =
      qobject_cast<QStandardItemModel*>(ui->image_list_view->model());
  image_model->clear();
  for (const auto& image_path : project->image_paths) {
    std::filesystem::path p(image_path);
    QStandardItem* image_path_item =
        new QStandardItem(QString::fromStdString(p.filename().string()));
    image_path_item->setData(QVariant::fromValue(image_path), Qt::UserRole);
    image_model->appendRow(image_path_item);
  }

  // videos
  auto video_model =
      qobject_cast<QStandardItemModel*>(ui->video_list_view->model());
  video_model->clear();
  for (const auto& video_path : project->video_paths) {
    std::filesystem::path p(video_path);
    QStandardItem* video_path_item =
        new QStandardItem(QString::fromStdString(p.filename().string()));
    video_path_item->setData(QVariant::fromValue(video_path), Qt::UserRole);
    video_model->appendRow(video_path_item);
  }
}

// 谱面列表双击事件
void MProjectController::on_map_list_view_doubleClicked(
    const QModelIndex& index) {
  auto map_model =
      qobject_cast<QStandardItemModel*>(ui->map_list_view->model());
  auto map = map_model->itemFromIndex(index)
                 ->data(Qt::UserRole)
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
    const QPoint& pos) {}

// 音频列表上下文菜单事件
void MProjectController::on_audio_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 图片列表上下文菜单事件
void MProjectController::on_image_list_view_customContextMenuRequested(
    const QPoint& pos) {}

// 视频列表上下文菜单事件
void MProjectController::on_video_list_view_customContextMenuRequested(
    const QPoint& pos) {}
