#ifndef MMM_MSKIN_HPP
#define MMM_MSKIN_HPP

#include <filesystem>
#include <mmm/project/AudioLoadCallback.hpp>
#include <nlohmann/json.hpp>

#include "mmm/project/TextureLoadCallback.hpp"

using json = nlohmann::json;

// 音效类型
enum class SoundEffectType : int32_t {
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
    std::string_view get_orbit_judge_texture();

    // 获取轨道底板纹理
    std::string_view get_orbit_bg_texture();

    // 获取选择框纹理
    std::string_view get_selected_border_texture(
        SelectBorderDirection direction);

    // 获取判定线的纹理
    std::string_view get_judgeline_texture();

    // 获取物件的纹理
    std::string_view get_object_texture(TexType type, ObjectStatus status);

    // 获取音效
    std::string_view get_sound_effect(SoundEffectType type);

   private:
    std::filesystem::path skinPath;
    // 基本信息
    std::string name;
    std::string author;
    // 字体
    std::string fontascii;
    std::string fontu8;

    // 音效表
    std::unordered_map<SoundEffectType, std::string> sound_effects;

    // 皮肤的根配置
    json skinRootCfg;
};

#endif  // MMM_MSKIN_HPP
