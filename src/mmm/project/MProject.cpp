#include <QDebug>
#include <filesystem>
#include <mmm/project/MProject.hpp>

// 构造MProject
MProject::MProject() {}

// 析构MProject
MProject::~MProject() = default;

// 打开项目
void MProject::open(std::string_view project_path) {
    // 打开路径
    auto path = std::filesystem::path(std::string(project_path));
    if (!std::filesystem::exists(path)) {
        qDebug() << "[" << project_path << "] 不存在";
        return;
    } else {
        // 直接调用纹理池载入文件夹内全部纹理

        for (auto it = std::filesystem::recursive_directory_iterator(path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (filename.ends_with("png") || filename.ends_with("jpg")) {
                project_image_table.insert(filename);
            } else if (filename.ends_with("mmm") || filename.ends_with("imd") ||
                       filename.ends_with("osu")) {
                // 载入谱面
                qDebug() << "需要载入谱面[" << filename << "]";
            }
        }
    }
}

// 关闭项目
void MProject::close() {}
