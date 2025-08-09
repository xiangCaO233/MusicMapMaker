#include <QDebug>
#include <filesystem>
#include <mmm/project/MProject.hpp>
#include <mmm/project/TextureLoadCallback.hpp>

// 构造MProject
MProject::MProject(TextureLoadCallback* texloadcbk) : callback(texloadcbk) {}

// 析构MProject
MProject::~MProject() {
    // 通知画布卸载纹理
    callback->need_unloadtexture_dir(project_path.generic_string());
}

// 打开项目
void MProject::open(std::string_view project_path_str) {
    // 打开路径
    project_path = std::filesystem::absolute(
        std::filesystem::path(std::string(project_path_str)));
    if (!std::filesystem::exists(project_path)) {
        qDebug() << "[" << project_path_str << "] 不存在";
        return;
    } else {
        // 在这直接调用纹理池载入文件夹内全部纹理
        callback->need_loadtexture_dir(project_path.generic_string());
        for (auto it =
                 std::filesystem::recursive_directory_iterator(project_path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (filename.ends_with("png") || filename.ends_with("jpg")) {
                project_image_table.insert(filename);
            } else if (filename.ends_with("mmm") || filename.ends_with("imd") ||
                       filename.ends_with("osu")) {
                // 载入谱面
                qDebug() << "需要载入谱面[" << filename << "]";
                auto map = std::make_unique<MMap>(filename);
                // 添加谱面到表中
                project_maps_table.try_emplace(map->base_metadata().name,
                                               std::move(map));
            }
        }
    }
}

// 关闭项目
void MProject::close() {}
