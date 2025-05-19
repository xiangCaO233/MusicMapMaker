#include "MapWorkProject.h"

#include <AudioManager.h>
#include <qdir.h>
#include <qlogging.h>

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
                               const char* name)
    : ppath(project_path) {
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
    config_xml_path = project_path / config_file_name;

    static const std::unordered_set<std::string> map_extention = {
        ".osu", ".imd", ".mmm"};
    static const std::unordered_set<std::string> audio_extention = {
        ".mp3", ".ogg", ".wav"};
    static const std::unordered_set<std::string> image_extention = {
        ".png", ".jpg", ".jpeg"};
    static const std::unordered_set<std::string> video_extention = {".mp4",
                                                                    ".mkv"};
    // 打开目录中可载入文件(.osu,.imd,.mmm,.mp3,.ogg,.wav,.png,.jpg,.jpeg,.mp4,.mkv)--到项目管理器
    // 遍历目录内容
    for (const auto& entry :
         std::filesystem::directory_iterator(project_path)) {
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
            if (extention == ".mmm") {
                map = std::make_shared<MMap>();
            }
            maps.back()->load_from_file(map_file_string.c_str());
            // 初始化画布时间位置
            map_canvasposes.try_emplace(map, 0.0);
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
    if (std::filesystem::exists(config_xml_path) &&
        std::filesystem::is_regular_file(config_xml_path)) {
        // 存在且为正常文件
        // 载入配置
        auto file_str = QDir(config_xml_path).canonicalPath().toStdString();
        xml_parse_result result = config_xml.load_file(file_str.c_str());

        if (!result) {
            XWARN("项目配置加载失败,重新创建配置");
            need_create_config = true;
        }
    } else {
        need_create_config = true;
    }
    // TODO(xiang 2025-04-16): 读取配置,或初始化配置
    // 现场载入配置
    if (need_create_config) {
        // 默认使用unknown设备
        devicename = "unknown output device";

        // 创建配置文件
        std::ofstream(config_xml_path.string()).close();

        // 项目属性
        project_root_node = config_xml.append_child("MMMProject");
        project_root_node.append_attribute("version");
        project_root_node.attribute("version").set_value("v0.1");

        // 初始化配置
        // 初始化项目名配置
        auto project_name_node = project_root_node.append_child("ProjectName");
        project_name_node.text().set(config.project_name);
        // 初始化音频配置
        audio_node = project_root_node.append_child("AudioConfig");
        // 初始化输出设备名配置
        auto device_name_node = audio_node.append_child("DeviceName");
        device_name_node.text().set(devicename);

        // 初始化输出音量配置
        audio_volume_node = audio_node.append_child("VolumeConfig");

        audio_volume_node.append_attribute("global").set_value(
            config.pglobal_volume);
        audio_volume_node.append_attribute("music").set_value(
            config.pmusic_volume);
        audio_volume_node.append_attribute("effect").set_value(
            config.peffect_volume);

        // 初始化尺寸配置
        sizeconfig_node = project_root_node.append_child("Sizes");
        sizeconfig_node.append_attribute("objwidth-scale")
            .set_value(config.object_width_ratio);
        sizeconfig_node.append_attribute("objheight-scale")
            .set_value(config.object_height_ratio);

        // 初始化画布配置
        canvasconfig_node = project_root_node.append_child("Canvas");
        canvasconfig_node.append_attribute("timeline-zoom")
            .set_value(config.timeline_zoom);
        canvasconfig_node.append_attribute("preview-time-scale")
            .set_value(config.preview_time_scale);
        canvasconfig_node.append_attribute("default-divisors")
            .set_value(config.default_divisors);
    } else {
        // 读取
        project_root_node = config_xml.child("MMMProject");

        // 读取项目名
        config.project_name =
            project_root_node.child("ProjectName").text().as_string();

        // 读取音频配置
        audio_node = project_root_node.child("AudioConfig");
        // 读取设备名
        devicename = audio_node.child("DeviceName").text().as_string();
        config.device = devicename;
        // 读取项目音频配置
        audio_volume_node = audio_node.child("VolumeConfig");
        config.pglobal_volume =
            audio_volume_node.attribute("global").as_float();
        config.pmusic_volume = audio_volume_node.attribute("music").as_float();
        config.peffect_volume =
            audio_volume_node.attribute("effect").as_float();

        // 读取尺寸配置
        sizeconfig_node = project_root_node.child("Sizes");
        config.object_width_ratio =
            sizeconfig_node.attribute("objwidth-scale").as_double();
        config.object_height_ratio =
            sizeconfig_node.attribute("objheight-scale").as_double();

        // 读取画布配置
        canvasconfig_node = project_root_node.child("Canvas");
        config.timeline_zoom =
            canvasconfig_node.attribute("timeline-zoom").as_double();
        config.preview_time_scale =
            canvasconfig_node.attribute("preview-time-scale").as_double();
        config.default_divisors =
            canvasconfig_node.attribute("default-divisors").as_double();

        XINFO("已读取项目配置");
    }
    // 使用项目目录
    XLogger::last_select_directory =
        QString::fromStdString(ppath.generic_string());
}

// 析构MapWorkProject
MapWorkProject::~MapWorkProject() {
    // 卸载音频
    for (const auto& path : audio_paths) {
        BackgroundAudio::unload_audio(path);
    }
    // 保存配置
    save_config();
}

// 添加新谱面
void MapWorkProject::add_new_map(const std::shared_ptr<MMap>& map) {
    maps.emplace_back(map);
    map_canvasposes.try_emplace(map, 0);
    map->project_reference = this;
}

// 设置项目音频输出设备
void MapWorkProject::set_audio_device(std::string& outdevicename) {
    devicename = outdevicename;
    config.device = devicename;
    audio_node.child("DeviceName").text().set(outdevicename);
}

// 保存配置
void MapWorkProject::save_config() {
    // 参数: 文件名, 缩进字符串, 格式化选项, 编码
    // PUGIXML_TEXT("  ") 用于跨平台处理缩进字符串字面量
    // 读取项目配置文件/没有则创建
    auto file_str = QDir(config_xml_path).absolutePath().toStdString();
    bool save_ok =
        config_xml.save_file(file_str.c_str(), PUGIXML_TEXT("    "),
                             pugi::format_indent, pugi::encoding_utf8);
    // pugi::format_default: 默认格式，通常会缩进
    // pugi::format_indent: 强制缩进 (通常已包含在 default 中)
    // pugi::format_raw: 不进行任何格式化，所有文本按原样输出
    // pugi::format_no_declaration: 不输出 <?xml ...?> 声明头
    // pugi::format_write_bom: 写入UTF-8 BOM (可选)

    if (save_ok) {
        qDebug() << "项目配置保存成功";
    } else {
        qWarning() << "项目配置保存成功";
    }
}
