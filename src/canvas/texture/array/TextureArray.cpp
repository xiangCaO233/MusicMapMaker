#include "TextureArray.h"

TextureArray::TextureArray() { pool_type = TexturePoolType::ARRARY; }

TextureArray::~TextureArray() {}
// 判满
bool TextureArray::is_full() {}
// 载入纹理
void TextureArray::load_texture(const char* resource_path) {}
