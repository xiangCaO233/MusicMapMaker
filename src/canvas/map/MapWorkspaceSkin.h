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

// 物件状态
enum ObjectStatus {
  COMMON,
  HOVER,
  SELECTED,
};

// 物件纹理类型
enum TexType {
  NOTE_HEAD,
  HOLD_BODY_VERTICAL,
  HOLD_BODY_HORIZONTAL,
  HOLD_END,
  NODE,
  SLIDE_END_LEFT,
  SLIDE_END_RIGHT,
};

// 选中框纹理的方向
enum SelectBorderDirection {
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

  std::string nomal_hit_effect_dir;
  std::string slide_hit_effect_dir;

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
};

#endif  // M_MAPWORKSPACE_PALLETE_H
