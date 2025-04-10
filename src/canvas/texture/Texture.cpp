#include "Texture.h"

#include <qdir.h>
#include <qnamespace.h>

// 构造
TextureInstace::TextureInstace() : is_atlas(true) {}

TextureInstace::TextureInstace(const char *path,
                               std::shared_ptr<BaseTexturePool> preference)
    : is_atlas(false), pool_reference(preference) {
  texture_image = QImage(path).convertToFormat(QImage::Format_RGBA8888);
  name = QDir(path).dirName().toStdString();
}

TextureInstace::~TextureInstace() = default;
