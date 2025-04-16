#ifndef M_MAPWORKPROJECT_H
#define M_MAPWORKPROJECT_H

#include <cstdint>
#include <filesystem>
#include <pugixml.hpp>
#include <string>
#include <vector>

#include "map/MMap.h"

struct ProjectConfig {
  // 项目名称
  std::string project_name;

  // 窗体尺寸策略
  uint32_t window_resolution_width;
  uint32_t window_resolution_height;

  // 画布尺寸策略
  uint32_t canvas_resolution_width;
  uint32_t canvas_resolution_height;
};

// 项目
class MapWorkProject {
 public:
  // 构造MapWorkProject
  explicit MapWorkProject(const std::filesystem::path& project_path,
                          const char* name = nullptr);

  // 析构MapWorkProject
  virtual ~MapWorkProject();

  // 配置
  ProjectConfig config;

  // 项目中谱的列表
  std::vector<MMap> maps;

  // xml配置文档
  pugi::xml_document config_xml;
};

#endif  // M_MAPWORKPROJECT_H
