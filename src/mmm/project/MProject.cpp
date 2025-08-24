#include <QDebug>
#include <filesystem>
#include <mmm/project/MProject.hpp>
#include <mmm/project/TextureLoadCallback.hpp>
#include <vector>

// 构造MProject
MProject::MProject(TextureLoadCallback* texloadcbk,
                   AudioLoadCallback* audioLoadcbk)
    : texcallback(texloadcbk), audiocallback(audioLoadcbk) {}

// 析构MProject
MProject::~MProject() { close(); }

// 打开项目
void MProject::open(std::string_view project_path_str) {
    if (is_opened.load()) {
        qDebug() << "此项目已经打开过";
        return;
    }
    // 打开路径
    project_path = std::filesystem::absolute(
        std::filesystem::path(std::string(project_path_str)));
    if (!std::filesystem::exists(project_path)) {
        qDebug() << "[" << project_path_str << "] 不存在";
        return;
    } else {
        // 在这直接调用纹理池回调载入文件夹内全部纹理
        texcallback->need_loadtexture_dir(project_path.generic_string());

        for (auto it =
                 std::filesystem::recursive_directory_iterator(project_path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (filename.ends_with(".png") || filename.ends_with(".jpg")) {
                project_image_table.insert(filename);
            } else if (
                // filename.ends_with(".mmm") ||
                filename.ends_with(".imd") || filename.ends_with(".osu")) {
                // 载入谱面
                qDebug() << "需要载入谱面[" << filename << "]";
                auto map = std::make_unique<MMap>(filename);
                map->bind_project(this);
                // 添加谱面到表中
                auto mapit =
                    project_maps_table
                        .try_emplace(map->base_metadata().name, std::move(map))
                        .first;
                auto map_maintrack = mapit->second->base_metadata()
                                         .main_audio_path.generic_string();
                auto map_track = audiocallback->loadBack(map_maintrack, true);
                // 添加谱面的主音轨
                qDebug() << "需要载入音频[" << map_maintrack << "]";
                project_main_audios_table.try_emplace(map_maintrack, map_track);
            } else if (filename.ends_with(".mp3") ||
                       filename.ends_with(".ogg") ||
                       filename.ends_with(".wav")) {
                if (!project_main_audios_table.contains(filename)) {
                    qDebug() << "需要载入音频[" << filename << "]";
                    // 直接通过音频轨道管理器回调载入音轨
                    auto track_weakptr = audiocallback->loadBack(filename);
                    project_normal_audios_table.try_emplace(filename,
                                                            track_weakptr);
                }
            } else if (filename.ends_with(".mp4") ||
                       filename.ends_with(".mkv")) {
                qDebug() << "需要载入视频[" << filename << "]";
                project_video_table.insert(filename);
            }
        }
        is_opened.store(true);
    }
}

// 关闭项目
void MProject::close() {
    if (is_closed) return;
    // 通知画布卸载纹理
    texcallback->need_unloadtexture_dir(project_path.generic_string());
    // 通知音频池卸载音轨
    is_closed.store(true);
}
