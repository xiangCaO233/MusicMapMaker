#ifndef M_GLOBAL_SETTINGS_H
#define M_GLOBAL_SETTINGS_H

// 时间格式
#include <cstdint>
#include <string>
enum class TimeFormat {
    MILLISECONDS,
    HHMMSSZZZ,
};

// 默认主题
enum class GlobalTheme {
    DARK,
    LIGHT,
};

// 设置
struct Settings {
    // 主题
    GlobalTheme global_theme;

    // 备份路径
    std::string backup_relative_path{".bak/"};

    // 备份文件保留数量
    int32_t backup_map_file_count{5};
};

#endif  // M_GLOBAL_SETTINGS_H
