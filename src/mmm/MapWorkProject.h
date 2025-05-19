#ifndef M_MAPWORKPROJECT_H
#define M_MAPWORKPROJECT_H

#include <qimage.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "map/MMap.h"

// 偏好的编辑方式
enum class EditMethodPreference : int32_t {
    MMM = 0,
    IVM = 1,
};

struct ProjectConfig {
    // 项目名称
    std::string project_name;

    // 默认的作者署名
    std::string def_author_name;

    // 使用的音频设备名
    std::string device;

    // 音量
    // 项目全局音量
    float pglobal_volume{.3f};
    // 项目音乐音量
    float pmusic_volume{.75f};
    // 项目效果音量
    float peffect_volume{.5f};

    // 物件缩放策略
    double object_width_ratio{1.0};
    double object_height_ratio{1.0};

    // 用户调节的时间线缩放-- n * 1px/1ms
    double timeline_zoom{1.0};

    // 预览区时间倍率:实际时间范围为当前时间范围*preview_time_scale
    double preview_time_scale{5.0};

    // 默认分拍策略
    int32_t default_divisors{2};

    // 总是保存为mmm文件
    bool alway_save_asmmm{true};

    // 项目偏好编辑方式
    EditMethodPreference edit_method{EditMethodPreference::IVM};
};

// 项目
class MapWorkProject {
    std::filesystem::path config_xml_path;

   public:
    // 构造MapWorkProject
    explicit MapWorkProject(const std::filesystem::path& project_path,
                            const char* name = nullptr);

    // 析构MapWorkProject
    virtual ~MapWorkProject();

    std::filesystem::path ppath;

    // 配置
    ProjectConfig config;

    // xml配置文档
    pugi::xml_document config_xml;
    // 根节点
    pugi::xml_node project_root_node;
    pugi::xml_node audio_node;
    pugi::xml_node audio_volume_node;
    pugi::xml_node sizeconfig_node;
    pugi::xml_node canvasconfig_node;

    // 项目中谱的列表
    std::vector<std::shared_ptr<MMap>> maps;

    // 项目中谱的显示时间位置
    std::unordered_map<std::shared_ptr<MMap>, double> map_canvasposes;

    // 当前项目所使用的音频输出设备
    std::string devicename;

    // 项目中音频的路径列表
    std::vector<std::string> audio_paths;

    // 项目中图片的路径列表
    std::vector<std::string> image_paths;

    // 项目中视频的路径列表
    std::vector<std::string> video_paths;

    // 添加新谱面
    void add_new_map(const std::shared_ptr<MMap>& map);

    // 设置项目音频输出设备
    void set_audio_device(std::string& outdevicename);

    // 保存配置
    void save_config();
};

#endif  // M_MAPWORKPROJECT_H
