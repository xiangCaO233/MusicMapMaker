#include "Texture.h"

// 构造
TextureInstace::TextureInstace() {}

TextureInstace::TextureInstace(const char *qrc_path) {
  texture_image = std::make_unique<QImage>(qrc_path);
}

TextureInstace::~TextureInstace() = default;
