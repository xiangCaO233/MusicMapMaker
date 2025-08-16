#ifndef MMM_PROJECTSERVICE_HPP
#define MMM_PROJECTSERVICE_HPP

#include <QObject>
#include <filesystem>

class MProject;
class MMap;

class ProjectService : public QObject {
    Q_OBJECT
   public slots:
    void openProject(const std::filesystem::path& project_path);
    void closeProject(MProject* project);

   signals:
    void activateProject(MProject* activated_project);
    void activateMap(MProject* current_project, MMap* activated_map);

   public:
    // 构造ProjectService
    ProjectService();
    // 析构ProjectService
    ~ProjectService() override;
};
#endif  // MMM_PROJECTSERVICE_HPP
