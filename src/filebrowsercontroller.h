#ifndef M_FILEBROWSER_CONTROLLER_H
#define M_FILEBROWSER_CONTROLLER_H

#include <qfilesystemmodel.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include <memory>

class MapWorkProject;

class MFileBrowserController : public QObject {
  Q_OBJECT
 public:
  // 构造MFileBrowserController
  MFileBrowserController();
  // 析构MFileBrowserController
  virtual ~MFileBrowserController();
 signals:
  void open_project(std::shared_ptr<MapWorkProject>& project);

 public slots:
  // 打开文件
  void on_menu_open_file(QString& file_path);

  // 重命名文件
  void on_menu_rename_file(QString& file_path);

  // 删除文件
  void on_menu_delete_file(QString& file_path);

  // 显示属性
  void on_menu_show_properties(QString& path);

  // 在文件夹内新建文件
  void on_menu_new_file_infolder(QString& file_path);

  // 打开文件夹
  void on_menu_open_folder(QFileSystemModel* model, QModelIndex& index);

  // 将文件夹打开为项目
  void on_open_folder_as_project(QString& file_path);

  // 地址栏确认事件
  void on_address_confirm_clicked();

  // 文件管理器双击事件
  void on_file_browser_treeview_doubleClicked(const QModelIndex& index);

  // 文件管理器返回事件
  void on_cdup_clicked();

  // 文件管理器向后返回事件
  void on_cdnext_clicked();

  // 文件管理器搜索事件
  void on_search_clicked();
};

#endif  // M_FILEBROWSER_CONTROLLER_H
