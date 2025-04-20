#include "Texture.h"

#include <qdir.h>
#include <qnamespace.h>

// 构造
TextureInstace::TextureInstace() : is_atlas(true) {}

TextureInstace::TextureInstace(const char* path,
                               std::shared_ptr<MTexturePool> preference)
    : poolreference(preference) {
  texture_image = QImage(path).convertToFormat(QImage::Format_RGBA8888);
  // TODO(xiang 2025-04-20): 更改带上父文件夹名防止文件名冲突
  auto fpath = QDir(path);
  auto file_name = fpath.dirName().toStdString();
  fpath.cdUp();
  auto parent_path_name = fpath.dirName().toStdString();

  name = parent_path_name + "/" + file_name;
}

TextureInstace::~TextureInstace() = default;
