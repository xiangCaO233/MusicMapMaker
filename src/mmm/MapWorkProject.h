#ifndef M_MAPWORKPROJECT_H
#define M_MAPWORKPROJECT_H

#include <filesystem>
#include <string>
#include <vector>

#include "map/MMap.h"

// 项目
class MapWorkProject {
  // 项目名称
  std::string project_name;

  // 项目中谱的列表
  std::vector<MMap> maps;

 public:
  // 构造MapWorkProject
  explicit MapWorkProject(const std::filesystem::path& project_path,
                          const char* name = nullptr);

  // 析构MapWorkProject
  virtual ~MapWorkProject();
};

#endif  // M_MAPWORKPROJECT_H
