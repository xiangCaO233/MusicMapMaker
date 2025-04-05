#include "TextureArray.h"

// 最大采样器层数
uint32_t TextureArray::max_texture_layer = 0;

TextureArray::TextureArray(QSize& size) : texture_size(size) {
  pool_type = TexturePoolType::ARRARY;
}

TextureArray::~TextureArray() {}

// 判满
bool TextureArray::is_full() { return texture_map.size() < max_texture_layer; }

// 载入纹理
void TextureArray::load_texture(const char* resource_path) {}
