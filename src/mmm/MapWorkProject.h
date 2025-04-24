#ifndef M_MAPWORKPROJECT_H
#define M_MAPWORKPROJECT_H

#include <qimage.h>

#include <cstdint>
#include <filesystem>
#include <memory>
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

  // 使用的音频设备名
  std::string device;
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
  std::vector<std::shared_ptr<MMap>> maps;

  // 当前项目所使用的音频输出设备
  std::string devicename;

  // 项目中音频的路径列表
  std::vector<std::string> audio_paths;

  // 项目中图片的路径列表
  std::vector<std::string> image_paths;

  // 项目中视频的路径列表
  std::vector<std::string> video_paths;

  // xml配置文档
  pugi::xml_document config_xml;

  // 设置项目音频输出设备
  void set_audio_device(std::string& outdevicename);
};

#endif  // M_MAPWORKPROJECT_H
