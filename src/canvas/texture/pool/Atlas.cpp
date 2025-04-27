#include "Atlas.h"

#include "../Texture.h"

// 构造Atlas
Atlas::Atlas(uint32_t width, uint32_t height)
    : atlas_width(width), atlas_height(height) {}

// 析构Atlas
Atlas::~Atlas() = default;

// 是否已满
bool Atlas::isfull() { return sub_textures.size() >= max_subimage_count; }

// 向纹理集添加纹理
bool Atlas::add_texture(std::shared_ptr<TextureInstace> &texture) {
  if (isfull()) return false;

  if (current_y + texture->height > atlas_height) return false;

  if (current_x + texture->width > atlas_width) {
    // 放下一行
    current_y += texture->height;
    current_x = 0;
  }

  // 下一行高度不足
  if (current_y + texture->height > atlas_height) return false;

  // 成功添加映射
  texture->woffset = current_x;
  texture->hoffset = current_y;
  current_x += texture->width;
  texture->texture_id = sub_textures.size();
  sub_textures.emplace_back(texture);

  return true;
}
