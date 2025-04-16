#include "MapWorkProject.h"

// 构造MapWorkProject
MapWorkProject::MapWorkProject(const std::filesystem::path& project_path,
                               const char* name) {
  // 初始化项目名
  if (name) {
    project_name = std::string(name);
  } else {
    // 使用项目目录名
    project_name = project_path.filename().string();
  }

  // TODO(xiang 2025-04-16): 实现从目录构造项目

  // 读取项目配置文件/没有则创建
  // 打开谱面文件
  // 加载谱面文件引用的全部音频
}

// 析构MapWorkProject
MapWorkProject::~MapWorkProject() = default;
