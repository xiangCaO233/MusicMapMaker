#include "TextureAtlas.h"

TextureAtlas::TextureAtlas() { pool_type = TexturePoolType::ATLAS; }

TextureAtlas::~TextureAtlas() {}

// 判满
bool TextureAtlas::is_full() {}

// 载入纹理
void TextureAtlas::load_texture(const char* resource_path) {}

// 生成纹理集
void TextureAtlas::creat_atlas() {}
