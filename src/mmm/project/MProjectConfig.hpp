#ifndef MMM_MPROJECTCONFIG_HPP
#define MMM_MPROJECTCONFIG_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

enum class EditMode {
    // mmm
    MMM,
    // ivm
    IVM,
};

struct AudioConfig {
    // 输出设备名
    std::string output_device_name;
    // 音轨列表
    std::vector<std::string> tracklist;
    // 音量
    float volume{.5f};
    // 拉伸倍率
    float stretch{1.f};
};

// 谱面内配置
struct MapConfig {
    // 背景暗化比例
    float darken{.2f};
    // 背景不透明度
    float alpha{1.f};
};

// 画布配置
struct CanvasConfig {
    // top,right,bottom,left
    glm::vec4 canvas_layout{.05f, .75f, .95f, .25f};
    // 判定线位置
    float judgeline_pos{.8f};
    // 选择框宽度
    float select_border_width{6.f};
    // 物件缩放
    float object_width_scale{1.f};
    float object_height_scale{1.f};
};

struct MProjectConfig {
    // 项目名称
    std::string project_name;

    // 偏好的编辑模式
    EditMode preference_editMode{EditMode::IVM};
    // 画布配置
    CanvasConfig canvas_config;

    // 音频配置
    // 全局音量
    float global_volume{1.f};

    // 特定配置
    AudioConfig maintrack_config;
    // AudioConfig effecttrack_config;
};

#endif  // MMM_MPROJECTCONFIG_HPP
