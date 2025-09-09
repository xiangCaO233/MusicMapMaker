#ifndef MMM_MSKIN_HPP
#define MMM_MSKIN_HPP

#include <QColor>
#include <filesystem>
#include <mmm/project/AudioLoadCallback.hpp>
#include <mmm/project/TextureLoadCallback.hpp>
#include <nlohmann/json.hpp>
#include <render/texture/TextureInfo.hpp>

using json = nlohmann::json;

enum class EffectTextureType : int32_t {
    NONE,
    // 正常击中
    NORMAL,
    // 滑键尾部
    SLIDE_END,
};

// 音效类型
enum class SoundEffectType : int32_t {
    NONE,
    // 正常击中
    COMMON_HIT = 0,
    // 滑动
    SLIDE = 1,
    // 长条按住
    HOLDING = 2,
    // 长条释放
    HOLD_RELEASE = 3,
};

// 物件状态
enum class ObjectStatus : int32_t {
    COMMON = 0,
    HOVER = 1,
    SELECTED = 2,
};

// 物件纹理类型
enum class TexType {
    HOLD_HEAD = 0,
    HOLD_BODY_VERTICAL = 1,
    HOLD_BODY_HORIZONTAL = 2,
    HOLD_END = 3,
    NODE = 4,
    SLIDE_END_LEFT = 5,
    SLIDE_END_RIGHT = 6,
    BACKGROUND = 7,
    ORBIT_BG = 8,
    JUDGE_ORBIT = 9,
    NORMAL_NOTE = 10,
};

// 选中框纹理的方向
enum class SelectBorderDirection {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
};

class MSkin {
   public:
    // 构造MSkin
    MSkin(std::string_view skin_path, AudioLoadCallback* audioLoadcbk,
          TextureLoadCallback* textureLoadcbk);

    // 析构MSkin
    virtual ~MSkin();

    // 皮肤基本字体族
    std::string_view fontFamilyASCII();

    // ascii无字符时使用
    std::string_view fontFamilyUTF8();

    // 获取轨道判定纹理
    TextureInfo get_orbit_judge_texture();

    // 获取轨道底板纹理
    TextureInfo get_orbit_bg_texture();

    // 获取选择框纹理
    TextureInfo get_selected_border_texture(SelectBorderDirection direction);

    // 获取判定线的纹理
    TextureInfo get_judgeline_texture();

    // 获取物件的纹理
    TextureInfo get_object_texture(TexType type, ObjectStatus status);

    // 获取音效
    std::string_view get_sound_effect(SoundEffectType type);

    // 获取时间分割线主题-(1/n-(颜色-宽度)列表)
    const std::vector<std::pair<QColor, int32_t>>& get_divisors_color_theme(
        int32_t d);

    // 清除缓存
    void clear_buffer();

    // 特效纹理目录
    std::string nomal_hit_effect_dir;
    std::string slide_hit_effect_dir;

    // 特效帧数量
    int32_t nomal_hit_effect_frame_count;
    double normal_hit_effect_duration;
    int32_t slide_hit_effect_frame_count;

   private:
    TextureLoadCallback* texcallback;
    std::filesystem::path skinPath;
    // 基本信息
    std::string name;
    std::string author;
    // 字体
    std::string fontascii;
    std::string fontu8;

    // 时间字体颜色
    QColor timeinfo_font_color;

    // 时间字体尺寸
    int32_t timeinfo_font_size;

    // 预览区背景色
    QColor preview_area_bg_color;

    // 皮肤配置
    // 皮肤的根配置
    json skinRootCfg;

    json texture_config;

    json bg_texture_config;

    json object_texture_config;

    json note_texture_config;

    json head_texture_config;

    json node_texture_config;

    json hold_texture_config;

    json vertical_holdbody_texture_config;

    json horizontal_holdbody_texture_config;

    json hold_end_texture_config;

    json left_slide_end_texture_config;

    json right_slide_end_texture_config;

    // 选中框纹理配置
    json selected_config;

    // 击中特效纹理配置
    json hit_effect_config;

    // 背景纹理缓存
    std::unordered_map<TexType, TextureInfo> bg_texture_buffer;

    // 音效表
    std::unordered_map<SoundEffectType, std::string> sound_effects;

    // 物件纹理缓存
    std::unordered_map<TexType, std::unordered_map<ObjectStatus, TextureInfo>>
        object_texture_buffer;

    std::vector<std::pair<QColor, int32_t>> default_divisor_theme;

    // 时间分割线主题-(1/n-(颜色-宽度)列表)
    std::unordered_map<int32_t, std::vector<std::pair<QColor, int32_t>>>
        divisors_color_theme;

    friend class MapCanvas;
};

#endif  // MMM_MSKIN_HPP
