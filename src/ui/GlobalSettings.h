#ifndef M_GLOBAL_SETTINGS_H
#define M_GLOBAL_SETTINGS_H

// 时间格式
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
};

#endif  // M_GLOBAL_SETTINGS_H
