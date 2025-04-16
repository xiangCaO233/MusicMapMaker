#ifndef PROJECTCONTROLLER_H
#define PROJECTCONTROLLER_H

#include <QWidget>

namespace Ui {
class MProjectController;
}

class MapWorkProject;

class MProjectController : public QWidget {
  Q_OBJECT

 public slots:
  // 新项目槽函数
  void new_project(std::shared_ptr<MapWorkProject> &project);

 public:
  explicit MProjectController(QWidget *parent = nullptr);

  ~MProjectController();

  // 制谱工程列表
  std::vector<std::shared_ptr<MapWorkProject>> project_list;

  // 制谱工程映射表
  std::unordered_map<std::string, std::shared_ptr<MapWorkProject>>
      project_mapping;

 private:
  Ui::MProjectController *ui;
};

#endif  // PROJECTCONTROLLER_H
