#include "mainwindow.h"

#include <qlogging.h>
#include <qobject.h>

#include <QVBoxLayout>
#include <memory>
#include <string>
#include <unordered_map>

#include "./ui_mainwindow.h"
#include "AudioManager.h"
#include "MapWorkspaceCanvas.h"
#include "colorful-log.h"
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

  setupsignals();
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

  // 设置工具栏按钮图标颜色
  mutil::set_toolbutton_svgcolor(ui->default_divisor_policy_toolbutton,
                                 ":/icons/stream.svg", file_button_color, 12,
                                 12);
  mutil::set_toolbutton_svgcolor(ui->bookmark_toolbutton,
                                 ":/icons/bookmark.svg", file_button_color, 12,
                                 12);
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
  mutil::set_button_svgcolor(ui->lock_edit_mode_button, ":/icons/lock-open.svg",
                             file_button_color, 16, 16);
}

// 设置信号
void MainWindow::setupsignals() {
  // 连接文件浏览器和项目管理器信号
  connect(ui->file_controller, &FileBrowserController::open_project,
          ui->project_controller, &MProjectController::new_project);

  // 连接项目选择map信号
  connect(ui->project_controller, &MProjectController::select_map, this,
          &MainWindow::project_controller_select_map);

  // 连接画布时间更新信号
  connect(ui->canvas, &MapWorkspaceCanvas::current_time_stamp_changed, this,
          &MainWindow::on_canvas_timestamp_changed);
}

// 选择音频输出设备事件
void MainWindow::on_audio_device_selector_currentIndexChanged(int index) {
  // 获取并设置当前选中的音频设备
  auto data = ui->audio_device_selector->currentData();
  current_use_device = data.value<std::shared_ptr<XOutputDevice>>();
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

// 模式锁定按钮状态切换事件
void MainWindow::on_lock_edit_mode_button_toggled(bool checked) {
  lock_mode_auto_switch = checked;
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
  mutil::set_button_svgcolor(
      ui->lock_edit_mode_button,
      (checked ? ":/icons/lock.svg" : ":/icons/lock-open.svg"),
      file_button_color, 16, 16);
}

// 选择页事件
void MainWindow::on_page_selector_currentTextChanged(const QString& text) {
  auto mapit = pagetext_maps_map.find(text);
  if (mapit == pagetext_maps_map.end()) {
    XWARN("无[" + text.toStdString() + "]页");
    return;
  }
  ui->canvas->switch_map(mapit->second);
}

// 关闭页面事件
void MainWindow::on_close_page_button_clicked() {
  auto current_text = ui->page_selector->currentText();
  auto mapit = pagetext_maps_map.find(current_text);
  if (mapit == pagetext_maps_map.end()) {
    XWARN("未发现[" + current_text.toStdString() + "]页");
    return;
  }
  // 移除当前选中的item
  ui->page_selector->removeItem(ui->page_selector->currentIndex());
  // 移除索引
  pagetext_maps_map.erase(mapit);

  // 更新画布
  if (pagetext_maps_map.empty()) {
    // 设置null
    ui->canvas->switch_map(nullptr);
  } else {
    // 随便选一个
    ui->canvas->switch_map(pagetext_maps_map.begin()->second);
  }
}

// 画布时间变化事件
void MainWindow::on_canvas_timestamp_changed(double time) {
  // 更新进度条
  auto maptime = (double)(ui->canvas->working_map->map_length);
  // XINFO("src maptime: " +
  // std::to_string(ui->canvas->working_map->map_length)); XINFO("maptime: " +
  // std::to_string(maptime));
  double ratio = time / maptime;
  // XINFO("ratio: " + std::to_string(ratio));
  ui->progress_slider->setValue(ratio * 10000.0);
}

// 进度条移动事件
void MainWindow::on_progress_slider_valueChanged(int value) {
  // 同步画布时间
  if (ui->canvas->is_paused()) {
    // 不暂停不允许调节时间
    if (ui->canvas->working_map) {
      double ratio = (double)value / 10000.0;
      auto maptime = (double)(ui->canvas->working_map->map_length);
      // XINFO("maptime: " + std::to_string(maptime));
      // XINFO("ratio: " + std::to_string(ratio));
      //  qDebug() << "value:" << value;
      ui->canvas->current_time_stamp = maptime * ratio;
    }
  }
}

// 项目控制器
// 项目控制器选择了map事件
void MainWindow::project_controller_select_map(std::shared_ptr<MMap>& map) {
  // 检查是否存在page映射
  auto page_text = QString::fromStdString(map->map_name);
  auto mapit = pagetext_maps_map.find(page_text);

  if (mapit == pagetext_maps_map.end()) {
    // 不存在,添加page映射
    pagetext_maps_map.try_emplace(page_text).first->second = map;

    // 添加selector item
    ui->page_selector->addItem(page_text);
  }

  ui->page_selector->setCurrentText(page_text);
}
