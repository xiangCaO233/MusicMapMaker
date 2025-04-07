#include "TextureAtlas.h"

#include "../../../log/colorful-log.h"
#include "texture/atlas/MaxRectsBinPack.h"

TextureAtlas::TextureAtlas(int width, int height, float expandrate)
    : packer(width, height, expandrate) {}

TextureAtlas::~TextureAtlas() {}

// 添加纹理
void TextureAtlas::add_texture(const char* resource_path) {}

// 生成纹理集
void TextureAtlas::pack() {
  if (is_packed) {
    XERROR("Atlas is already packed");
  } else {
    // TODO(xiang 2025-04-07): 完成打包
    is_packed = true;
  }
}
