#include "Texture.h"

#include <qdir.h>
#include <qnamespace.h>
#include <qobject.h>

// 构造
TextureInstace::TextureInstace() : is_atlas(true) {}

TextureInstace::TextureInstace(std::filesystem::path& path,
                               std::shared_ptr<MTexturePool> preference)
    : poolreference(preference) {
  load_from_file(path);
}

TextureInstace::TextureInstace(const char* path,
                               std::shared_ptr<MTexturePool> preference)
    : poolreference(preference) {
  auto p = std::filesystem::path(path);
  load_from_file(p);
}

TextureInstace::~TextureInstace() = default;

// 从文件加载
void TextureInstace::load_from_file(std::filesystem::path& file_path) {
  auto fpath = QDir(QString::fromStdString(file_path.string()));
  auto file_name = fpath.dirName().toStdString();
  fpath.cdUp();
  auto parent_path_name = fpath.dirName().toStdString();
  name = parent_path_name + "/" + file_name;

  texture_image =
      QImage(file_path.c_str()).convertToFormat(QImage::Format_RGBA8888);
}
