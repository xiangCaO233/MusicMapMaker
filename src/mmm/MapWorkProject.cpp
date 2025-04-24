#include "MapWorkProject.h"

#include <AudioManager.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_set>

#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "mmm/map/osu/OsuMap.h"
#include "mmm/map/rm/RMMap.h"

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

  XINFO("构建项目");

  // 读取项目配置文件/没有则创建
  auto config_file_name = config.project_name + ".xml";
  auto config_path = project_path / config_file_name;

  static const std::unordered_set<std::string> map_extention = {".osu", ".imd",
                                                                ".mc"};
  static const std::unordered_set<std::string> audio_extention = {
      ".mp3", ".ogg", ".wav"};
  static const std::unordered_set<std::string> image_extention = {
      ".png", ".jpg", ".jpeg"};
  static const std::unordered_set<std::string> video_extention = {".mp4",
                                                                  ".mkv"};
  // 打开目录中可载入文件(.osu,.imd,.mp3,.ogg,.wav,.png,.jpg,.jpeg,.mp4,.mkv)--到项目管理器
  // 遍历目录内容
  for (const auto& entry : std::filesystem::directory_iterator(project_path)) {
    auto abspath = std::filesystem::weakly_canonical(
        std::filesystem::absolute(entry.path()));
    const auto extention = abspath.extension().string();
    if (map_extention.find(extention) != map_extention.end()) {
      // 谱面文件--预加载
      maps.emplace_back();
      const auto map_file_string = abspath.string();
      auto& map = maps.back();
      if (extention == ".osu") {
        map = std::make_shared<OsuMap>();
      }
      if (extention == ".imd") {
        map = std::make_shared<RMMap>();
      }
      maps.back()->load_from_file(map_file_string.c_str());
    } else if (audio_extention.find(extention) != audio_extention.end()) {
      // 音频文件
      audio_paths.emplace_back(abspath.string());
      // 加载音频
      BackgroundAudio::loadin_audio(abspath.string());
    } else if (image_extention.find(extention) != image_extention.end()) {
      // 图片文件
      image_paths.emplace_back(abspath.string());
    } else if (video_extention.find(extention) != video_extention.end()) {
      // 视频文件
      video_paths.emplace_back(abspath.string());
    }
  }

  // 设置谱面的音频引用
  for (const auto& map : maps) {
    auto filename = map->audio_file_abs_path.filename().string();
    // 去除头部空格
    filename.erase(
        filename.begin(),
        std::find_if(filename.begin(), filename.end(),
                     [](unsigned char ch) { return !std::isspace(ch); }));
    // 去除尾部空格
    filename.erase(
        std::find_if(filename.rbegin(), filename.rend(),
                     [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
        filename.end());
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

  // 默认使用unknown设备
  devicename = "unknown output device";
}

// 析构MapWorkProject
MapWorkProject::~MapWorkProject() {
  // 卸载音频
  for (const auto& path : audio_paths) {
    BackgroundAudio::unload_audio(path);
  }
};

// 设置项目音频输出设备
void MapWorkProject::set_audio_device(std::string& outdevicename) {
  devicename = outdevicename;
}
