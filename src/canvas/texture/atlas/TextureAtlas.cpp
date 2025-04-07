#include "TextureAtlas.h"

#include "../../../log/colorful-log.h"
#include "texture/atlas/MaxRectsBinPack.h"

TextureAtlas::TextureAtlas(int width, int height, float expandrate)
    : packer(width, height, expandrate) {
  buffer.atlas_width = width;
  buffer.atlas_height = height;
}

TextureAtlas::~TextureAtlas() {}

// 添加纹理
void TextureAtlas::add_texture(const char* resource_path) {}

// 生成纹理集
void TextureAtlas::pack() {
  if (is_packed) {
    XERROR("Atlas is already packed");
  } else {
    // TODO(xiang 2025-04-07): 完成打包
    XINFO("打包纹理集");
    is_packed = true;
  }
}
// 获取尺寸
QSize TextureAtlas::size() { return {packer.binWidth, packer.binWidth}; }

// 纹理集是否已满
bool TextureAtlas::is_full() {
  return sub_images.size() >= max_subimage_count ||
         packer.binWidth >= def_max_size.width() &&
             packer.binHeight >= def_max_size.height();
}
