#ifndef MFILEBROWSERCONTROLLER_H
#define MFILEBROWSERCONTROLLER_H

#include <QWidget>

#include "mmm/MapWorkProject.h"

enum class GlobalTheme;

namespace Ui {
class FileBrowserController;
}

class FileBrowserController : public QWidget {
  Q_OBJECT

 public:
  explicit FileBrowserController(QWidget *parent = nullptr);
  ~FileBrowserController();

  // 使用主题
  void use_theme(GlobalTheme theme);

 signals:
  // 打开工程信号
  void open_project(std::shared_ptr<MapWorkProject> &project);

 private slots:
  // 上下文菜单槽
  void on_file_browser_treeview_customContextMenuRequested(const QPoint &pos);

 private:
  Ui::FileBrowserController *ui;
};

#endif  // MFILEBROWSERCONTROLLER_H
