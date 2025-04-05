#include "TexturePool.h"

TexturePool::TexturePool() { pool_type = TexturePoolType::BASE_POOL; }

TexturePool::~TexturePool() {}

// 判满
bool TexturePool::is_full() {}

// 载入纹理
void TexturePool::load_texture(const char* resource_path) {}
