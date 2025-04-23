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

  int width, height, nrChannels;
  unsigned char* data =
      stbi_load(file_path.string().c_str(), &width, &height, &nrChannels, 0);

  XINFO("stb load res:" + file_name + "[w:" + std::to_string(width) +
        ",h:" + std::to_string(height) +
        ",channels:" + std::to_string(nrChannels) + "]");

  if (data) {
    QImage::Format format = QImage::Format_Invalid;

    // 根据通道数选择正确的QImage格式
    switch (nrChannels) {
      case 1:
        format = QImage::Format_Grayscale8;
        break;
      case 3:
        format = QImage::Format_RGB888;
        break;
      case 4:
        format = QImage::Format_RGBA8888;
        break;
      default:
        // 不支持的通道数
        stbi_image_free(data);
        // 处理错误或返回空QImage
        texture_image = QImage();
        return;
    }

    // 创建QImage并复制数据
    QImage image(data, width, height, width * nrChannels, format);

    // 由于QImage不会自动释放data，我们需要做一个深拷贝
    QImage result = image.copy();
    texture_image = result.convertToFormat(QImage::Format_RGBA8888);

    // 释放stb_image分配的内存
    stbi_image_free(data);

  } else {
    texture_image = QImage();
    return;
  }
}
