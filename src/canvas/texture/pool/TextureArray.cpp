#include "TextureArray.h"

#include "../../../log/colorful-log.h"

// 最大采样器层数
uint32_t TextureArray::max_texture_layer = 0;

TextureArray::TextureArray(GLCanvas* canvas, QSize size)
    : BaseTexturePool(canvas), texture_size(size) {
  XINFO("构造纹理数组纹理池");
  pool_type = TexturePoolType::ARRARY;
}

TextureArray::~TextureArray() {}

// 判满
bool TextureArray::is_full() { return texture_map.size() < max_texture_layer; }

// 载入纹理
int TextureArray::load_texture(std::shared_ptr<TextureInstace> texture) {
  if (is_full()) {
    return POOL_FULL;
  }
  if (texture->texture_image.size() != texture_size) {
    return EXPECTED_SIZE;
  }
  return SUCCESS;
}

// 完成纹理池构造
void TextureArray::finalize() { XINFO("完成处理纹理数组纹理池"); }

// 使用纹理数组池
// 使用纹理集请在use后更新usage的uniform
void TextureArray::use() {}
