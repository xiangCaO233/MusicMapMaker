#ifndef MMM_MPROJECTCONFIG_HPP
#define MMM_MPROJECTCONFIG_HPP

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

struct CanvasConfig {
    float object_width_scale{1.f};
    float object_height_scale{1.f};
};

struct MProjectConfig {
    // 项目名称
    std::string project_name;

    // 偏好的编辑模式
    EditMode preference_editMode{EditMode::IVM};

    // 音频配置
    // 全局音量
    float global_volume{1.f};
    // 特定配置
    AudioConfig maintrack_config;
    AudioConfig effecttrack_config;

    // 画布配置
    CanvasConfig canvas_config;
};

#endif  // MMM_MPROJECTCONFIG_HPP
