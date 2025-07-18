#include "Texture.h"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qimage.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>

#include <filesystem>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

// 构造
TextureInstace::TextureInstace() {}

TextureInstace::TextureInstace(std::filesystem::path& relative_path,
                               std::filesystem::path& path,
                               std::shared_ptr<MTexturePool> preference)
    : poolreference(preference) {
    load_from_file(relative_path, path);
}

TextureInstace::TextureInstace(const char* relative_path, const char* path,
                               std::shared_ptr<MTexturePool> preference)
    : poolreference(preference) {
    auto p = std::filesystem::path(path);
    auto rp = std::filesystem::path(relative_path);
    load_from_file(rp, p);
}

TextureInstace::~TextureInstace() {
    // 释放stb_image分配的内存
    stbi_image_free(data);
};

// 从文件加载
void TextureInstace::load_from_file(std::filesystem::path& relative_path,
                                    std::filesystem::path& file_path) {
    auto rp = std::filesystem::relative(file_path, relative_path);

    if (file_path.root_path() == relative_path) {
        name = QString::fromStdString(file_path.string()).toStdString();
    } else {
        name = QString::fromStdString(rp.string()).toStdString();
    }
    std::replace(name.begin(), name.end(), '\\', '/');

    data = stbi_load(file_path.string().c_str(), &width, &height, &channels, 4);

    // XINFO("stb load res:" + file_name + "[w:" + std::to_string(width) + ",h:"
    // +
    //       std::to_string(height) + ",channels:" + std::to_string(channels) +
    //       "]");

    if (!data) {
        // XWARN("加载纹理失败");
        return;
    }
}
