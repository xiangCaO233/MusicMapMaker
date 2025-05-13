#ifndef M_MAPWORKSPACE_PALLETE_H
#define M_MAPWORKSPACE_PALLETE_H

#include <qcolor.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../texture/Texture.h"

class MapWorkspaceCanvas;

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
    NOTE_HEAD = 0,
    HOLD_BODY_VERTICAL = 1,
    HOLD_BODY_HORIZONTAL = 2,
    HOLD_END = 3,
    NODE = 4,
    SLIDE_END_LEFT = 5,
    SLIDE_END_RIGHT = 6,
};

// 选中框纹理的方向
enum class SelectBorderDirection {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
};

// 击打特效帧
struct HitEffectFrame {
    std::shared_ptr<TextureInstace> effect_texture;
    QRectF effect_bound;
};

// 工作区皮肤
class MapWorkspaceSkin {
   public:
    // 构造MapWorkspacePallete
    MapWorkspaceSkin(MapWorkspaceCanvas* canvas);

    // 析构MapWorkspacePallete
    ~MapWorkspaceSkin();

    // 画布引用
    MapWorkspaceCanvas* cvs;

    // 皮肤名
    std::string skinname;

    // 作者名
    std::string authorname;

    // 全局字体
    std::string font_family;

    // 时间字体颜色
    QColor timeinfo_font_color;

    // 时间字体尺寸
    int32_t timeinfo_font_size;

    // 预览区背景色
    QColor preview_area_bg_color;

    // 皮肤配置
    json skin_config;

    json texture_config;

    json bg_texture_config;

    json object_texture_config;

    // 物件纹理缓存
    std::unordered_map<
        TexType,
        std::unordered_map<ObjectStatus, std::shared_ptr<TextureInstace>>>
        object_texture_buffer;

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

    // 音效表
    std::unordered_map<SoundEffectType, std::string> sound_effects;

    // 特效纹理目录
    std::string nomal_hit_effect_dir;
    std::string slide_hit_effect_dir;

    // 特效帧数量
    int32_t nomal_hit_effect_frame_count;
    int32_t slide_hit_effect_frame_count;

    // 时间分割线主题-(1/n-(颜色-宽度)列表)
    std::unordered_map<int32_t, std::vector<std::pair<QColor, int32_t>>>
        divisors_color_theme;

    // 载入皮肤
    void load_skin(std::filesystem::path& skin_path);

    // 获取背景的纹理
    std::shared_ptr<TextureInstace>& get_skin_background_texture();

    // 获取精灵的纹理
    std::shared_ptr<TextureInstace>& get_sprite_texture();

    // 获取选择框纹理
    std::shared_ptr<TextureInstace>& get_selected_border_texture(
        SelectBorderDirection direction);

    // 获取判定线的纹理
    std::shared_ptr<TextureInstace>& get_judgeline_texture();

    // 获取物件的纹理
    std::shared_ptr<TextureInstace>& get_object_texture(TexType type,
                                                        ObjectStatus status);

    // 获取音效
    std::string& get_sound_effect(SoundEffectType type);
};

#endif  // M_MAPWORKSPACE_PALLETE_H
