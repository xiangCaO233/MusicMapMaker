#include "TextureAtlas.h"

#include "texture/atlas/MaxRectsBinPack.h"

TextureAtlas::TextureAtlas(int width, int height, float expandrate)
    : packer(width, height, expandrate) {
  pool_type = TexturePoolType::ATLAS;
}

TextureAtlas::~TextureAtlas() {}

// 判满
bool TextureAtlas::is_full() { return is_packed; }

// 载入纹理
void TextureAtlas::load_texture(const char* resource_path) {}

// 生成纹理集
void TextureAtlas::creat_atlas() {}
