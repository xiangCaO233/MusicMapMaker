#include "Texture.h"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qimage.h>
#include <qnamespace.h>
#include <qobject.h>

#include <filesystem>
#include <string>

#include "../../log/colorful-log.h"
#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

// 构造
TextureInstace::TextureInstace() {}

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

TextureInstace::~TextureInstace() {
  // 释放stb_image分配的内存
  stbi_image_free(data);
};

// 从文件加载
void TextureInstace::load_from_file(std::filesystem::path& file_path) {
  auto fpath = QDir(QString::fromStdString(file_path.string()));
  auto file_name = fpath.dirName().toStdString();
  fpath.cdUp();
  auto parent_path_name = fpath.dirName().toStdString();
  name = parent_path_name + "/" + file_name;

  data = stbi_load(file_path.string().c_str(), &width, &height, &channels, 4);

  XINFO("stb load res:" + file_name + "[w:" + std::to_string(width) + ",h:" +
        std::to_string(height) + ",channels:" + std::to_string(channels) + "]");

  if (!data) {
    XWARN("加载纹理失败");
    return;
  }
}
