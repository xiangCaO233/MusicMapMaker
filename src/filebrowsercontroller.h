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
  void on_open_file(QString& file_path);
  void on_rename_file(QString& file_path);
  void on_delete_file(QString& file_path);
  void on_show_properties(QString& path);
  void on_new_file_infolder(QString& file_path);
  void on_open_folder(QFileSystemModel* model, QModelIndex& index);
  void on_open_folder_as_project(QString& file_path);
};

#endif  // M_FILEBROWSER_CONTROLLER_H
