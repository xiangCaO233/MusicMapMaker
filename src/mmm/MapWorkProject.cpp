#include "MapWorkProject.h"

#include <filesystem>
#include <fstream>
#include <ostream>

#include "colorful-log.h"

using namespace pugi;

// 构造MapWorkProject
MapWorkProject::MapWorkProject(const std::filesystem::path& project_path,
                               const char* name) {
  // 初始化项目名
  if (name) {
    config.project_name = std::string(name);
  } else {
    // 使用项目目录名
    config.project_name = project_path.filename().string();
  }

  // TODO(xiang 2025-04-16): 实现从目录构造项目
  XINFO("构建项目");

  // 读取项目配置文件/没有则创建
  auto config_file_name = config.project_name + ".xml";
  auto config_path = project_path / config_file_name;

  // 打开目录中可载入文件(.osu,.imd,.mp3,.ogg,.wav,.png,.jpg,.jpeg,.mp4,.mkv)--到项目管理器
  // 遍历目录内容
  for (const auto& entry : std::filesystem::directory_iterator(project_path)) {
    XINFO("检查文件:[" + entry.path().string() + "]");
  }

  // 检查配置文件
  bool need_create_config{false};
  if (std::filesystem::exists(config_path) &&
      std::filesystem::is_regular_file(config_path)) {
    // 存在且为正常文件
    // 载入配置
    xml_parse_result result =
        config_xml.load_file(config_path.string().c_str());
    if (!result) {
      XWARN("项目配置加载失败,重新创建配置");
      need_create_config = true;
    }
  } else {
    need_create_config = true;
  }
  // TODO(xiang 2025-04-16): 读取配置,或初始化配置
  if (need_create_config) {
    // 创建配置文件
    std::ofstream(config_path.string()).close();
    // 现场载入配置
    config_xml.load_file(config_path.string().c_str());
    // 初始化配置
  } else {
    // 读取
  }
}

// 析构MapWorkProject
MapWorkProject::~MapWorkProject() = default;
