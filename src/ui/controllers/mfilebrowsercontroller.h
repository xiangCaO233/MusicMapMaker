#ifndef MFILEBROWSERCONTROLLER_H
#define MFILEBROWSERCONTROLLER_H

#include <QWidget>
#include <stack>

#include "../../mmm/MapWorkProject.h"

enum class GlobalTheme;

namespace Ui {
class FileBrowserController;
}

class FileBrowserController : public QWidget {
  Q_OBJECT

 public:
  explicit FileBrowserController(QWidget *parent = nullptr);
  ~FileBrowserController();

  // 缓存目录
  std::stack<QString> last_path_stack;
  std::stack<QString> next_path_stack;

  // 音频管理器引用
  std::shared_ptr<XAudioManager> audio_manager_reference;

  // 使用主题
  void use_theme(GlobalTheme theme);

  // 切换目录
  void cd(const QString &path);

 signals:
  // 打开工程信号
  void open_project(std::shared_ptr<MapWorkProject> &project);

 private slots:
  // 文件列表上下文菜单槽
  void on_file_browser_treeview_customContextMenuRequested(const QPoint &pos);

  // 文件列表双击事件
  void on_file_browser_treeview_doubleClicked(const QModelIndex &index);

  // 返回上一目录
  void on_cdup_clicked();

  // 返回下一目录
  void on_cdnext_clicked();

  // 返回父级目录
  void on_cdparent_clicked();

  // 地址栏编辑回车按下
  void on_address_line_returnPressed();

  // 地址确认按键事件
  void on_address_confirm_clicked();

  // 搜索按键事件
  void on_search_clicked();

 private:
  Ui::FileBrowserController *ui;
};

#endif  // MFILEBROWSERCONTROLLER_H
