#include "Atlas.h"

#include "../Texture.h"
#include "MaxRectsBinPack.h"

// 构造Atlas
Atlas::Atlas(uint32_t width, uint32_t height) : packer(width, height, 1.0f) {}

// 析构Atlas
Atlas::~Atlas() = default;

// 是否已满
bool Atlas::isfull() {
  return sub_textures.size() >= max_subimage_count || packer.full;
}

// 向纹理集添加纹理
bool Atlas::add_texture(std::shared_ptr<TextureInstace> &texture) {
  if (isfull()) return false;

  auto res = packer.Insert(texture, MaxRectsBinPack::RectBestAreaFit);
  if (res) {
    // 成功添加映射
    texture->texture_id = sub_textures.size();
    sub_textures.emplace_back(texture);
  }
  return res;
}
