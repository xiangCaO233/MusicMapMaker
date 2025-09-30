#include <log/colorful-log.h>
#include <trackmanager.h>

#include <QDebug>
#include <filesystem>
#include <mmm/map/MMap.hpp>
#include <mmm/project/MProject.hpp>
#include <mmm/project/TextureLoadCallback.hpp>
#include <string>

// 构造MProject
MProject::MProject(TextureLoadCallback* texloadcbk,
                   AudioLoadCallback* audioLoadcbk)
    : texcallback(texloadcbk), audiocallback(audioLoadcbk) {}

// 析构MProject
MProject::~MProject() { close(); }

// 打开项目
void MProject::open(std::string_view project_path_str) {
    if (is_opened.load()) {
        XWARN("此项目已经打开过");
        return;
    }
    // 打开路径
    project_path = std::filesystem::absolute(
        std::filesystem::path(std::string(project_path_str)));
    if (!std::filesystem::exists(project_path)) {
        XWARN("[" + std::string(project_path_str) + "] 不存在");
        return;
    } else {
        // 在这直接调用纹理池回调载入文件夹内全部纹理
        texcallback->need_loadtexture_dir(project_path.generic_string());
        std::unordered_set<std::filesystem::path> maps;
        std::unordered_set<std::filesystem::path> audios;
        // 载入项目目录
        for (auto it = std::filesystem::directory_iterator(project_path);
             it != std::filesystem::directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (filename.ends_with(".png") || filename.ends_with(".jpg") ||
                filename.ends_with(".jpeg")) {
                project_image_table.insert(filename);
            } else if (filename.ends_with(".mmm") ||
                       filename.ends_with(".imd") ||
                       filename.ends_with(".osu")) {
                // 载入谱面
                XINFO("需要载入谱面[" + filename + "]");
                if (!maps.contains(it->path())) maps.insert(it->path());

            } else if (filename.ends_with(".mp3") ||
                       filename.ends_with(".ogg") ||
                       filename.ends_with(".wav")) {
                if (audios.contains(it->path())) audios.insert(it->path());
            } else if (filename.ends_with(".mp4") ||
                       filename.ends_with(".mkv")) {
                XINFO("需要载入视频[" + filename + "]");
                project_video_table.insert(filename);
            } else if (filename.ends_with(".mproject")) {
                XINFO("发现配置文件[" + filename + "]");
                auto result = config_doc.load_file(filename.c_str());
                if (!result) {
                    XWARN("项目配置解析失败: " +
                          std::string(result.description()));
                }
            }
        }

        // 先加载所有map
        for (const auto& mappath : maps) {
            auto filename = mappath.generic_string();
            auto map = std::make_unique<MMap>(
                static_cast<TrackManager*>(audiocallback), filename);
            map->bind_project(this);
            // 添加谱面到表中
            auto mapit =
                project_maps_table
                    .try_emplace(map->base_metadata().name, std::move(map))
                    .first;
            auto map_maintrack =
                mapit->second->base_metadata().main_audio_path.generic_string();

            if (!project_main_audios_table.contains(map_maintrack)) {
                // 添加谱面的主音轨
                XINFO("map加载需要载入音频[" + map_maintrack + "]");
                project_main_audios_table.try_emplace(map_maintrack);
                auto map_track = audiocallback->loadBack(map_maintrack, true);
                project_main_audios_table[map_maintrack] = map_track;
            }
        }

        // 再筛选加载剩余的音频
        for (const auto& audiopath : audios) {
            auto filename = audiopath.generic_string();
            if (!project_main_audios_table.contains(filename)) {
                XINFO("目录加载需要载入其他音频[" + filename + "]");
                project_normal_audios_table.try_emplace(filename);
                // 直接通过音频轨道管理器回调载入音轨
                auto track_weakptr = audiocallback->loadBack(filename);
                project_normal_audios_table[filename] = track_weakptr;
            }
        }

        // 更新所有map的配置ui
        for (const auto& [name, map] : project_maps_table) {
            map->update_configui();
        }
        update_configdoc(false);
        is_opened.store(true);
    }
}

// 添加音轨
void MProject::add_audio_track(const std::string& file,
                               std::shared_ptr<ice::AudioTrack> track,
                               bool main_track) {
    if (main_track) {
        project_main_audios_table[file] = track;
    } else {
        project_normal_audios_table[file] = track;
    }
}

// 添加map
void MProject::add_map(std::unique_ptr<MMap> map) {
    map->bind_project(this);
    // 添加谱面到表中
    if (map->base_metadata().name.empty()) {
        auto& basemeta = map->base_metadata();
        // 自动生成mapname
        basemeta.name = "[mmm] " + basemeta.artist_unicode + "-" +
                        basemeta.title_unicode + "(" + basemeta.author + ") [" +
                        std::to_string(basemeta.track_count) + "k] - " +
                        basemeta.version;
    }
    auto mapit = project_maps_table
                     .try_emplace(map->base_metadata().name, std::move(map))
                     .first;
    auto map_maintrack_name =
        mapit->second->base_metadata().main_audio_path.generic_string();

    if (!project_main_audios_table.contains(map_maintrack_name)) {
        // 添加谱面的主音轨
        XINFO("新map加载需要载入音频[" + map_maintrack_name + "]");
        // 检查是否载入过
        auto wtrack =
            static_cast<TrackManager*>(audiocallback)
                ->get_track(QString::fromStdString(map_maintrack_name));
        if (auto track = wtrack.lock()) {
            // 载入过,只需添加映射
            project_main_audios_table.try_emplace(map_maintrack_name, track);
        } else {
            project_main_audios_table.try_emplace(map_maintrack_name);
            auto map_track = audiocallback->loadBack(map_maintrack_name, true);
            project_main_audios_table[map_maintrack_name] = map_track;
        }
    }
}

// 更新配置文档
void MProject::update_configdoc(bool from_config) {
    XINFO("开始更新项目配置");
    // 根节点
    auto root_node = config_doc.child("mproject");
    if (!root_node) root_node = config_doc.append_child("mproject");

    // 项目名称节点
    auto project_name_node = root_node.child("name");
    if (!project_name_node) {
        // 如果节点不存在// 则创建它
        project_name_node = root_node.append_child("name");
        if (!from_config) {
            // 首次加载使用文件夹名作为项目名
            project_name_node.text().set(
                project_path.filename().generic_string());
        }
    } else {
        XINFO("项目名称:" + std::string(project_name_node.text().as_string()));
        if (from_config) {
            XINFO("正在保存配置");
        } else {
            XINFO("正在读取配置");
        }
    }
    if (from_config) {
        // 更新为实时配置中的值
        project_name_node.text().set(project_config.project_name);
    } else {
        // 在读取-从节点中获取值用于更新配置结构
        project_config.project_name = project_name_node.text().as_string();
    }

    // 画布配置节点
    auto canvas_layout_node = root_node.child("canvas-layout");
    if (!canvas_layout_node)
        canvas_layout_node = root_node.append_child("canvas-layout");

    // 主轨道布局配置子节点
    auto maintrack_lauout_node = canvas_layout_node.child("maintrack-layout");
    if (!maintrack_lauout_node)
        maintrack_lauout_node =
            canvas_layout_node.append_child("maintrack-layout");
    auto topratio_attr = maintrack_lauout_node.attribute("top");
    auto rightratio_attr = maintrack_lauout_node.attribute("right");
    auto bottomratio_attr = maintrack_lauout_node.attribute("bottom");
    auto leftratio_attr = maintrack_lauout_node.attribute("left");
    if (!topratio_attr) {
        topratio_attr = maintrack_lauout_node.append_attribute("top");
        topratio_attr = .05f;
    }
    if (!rightratio_attr) {
        rightratio_attr = maintrack_lauout_node.append_attribute("right");
        rightratio_attr = .75f;
    }
    if (!bottomratio_attr) {
        bottomratio_attr = maintrack_lauout_node.append_attribute("bottom");
        bottomratio_attr = .95f;
    }
    if (!leftratio_attr) {
        leftratio_attr = maintrack_lauout_node.append_attribute("left");
        leftratio_attr = .25f;
    }
    if (from_config) {
        topratio_attr = project_config.canvas_config.canvas_layout.x;
        rightratio_attr = project_config.canvas_config.canvas_layout.y;
        bottomratio_attr = project_config.canvas_config.canvas_layout.z;
        leftratio_attr = project_config.canvas_config.canvas_layout.w;
    } else {
        project_config.canvas_config.canvas_layout.x = topratio_attr.as_float();
        project_config.canvas_config.canvas_layout.y =
            rightratio_attr.as_float();
        project_config.canvas_config.canvas_layout.z =
            bottomratio_attr.as_float();
        project_config.canvas_config.canvas_layout.w =
            leftratio_attr.as_float();
    }
    // 判定线位置配置子节点
    auto judgeline_pos_node = canvas_layout_node.child("judgeline");
    if (!judgeline_pos_node) {
        judgeline_pos_node = canvas_layout_node.append_child("judgeline");
        judgeline_pos_node.text().set(.8f);
    }
    if (from_config)
        judgeline_pos_node.text().set(
            project_config.canvas_config.judgeline_pos);
    else
        project_config.canvas_config.judgeline_pos =
            judgeline_pos_node.text().as_float();

    // 物件缩放配置子节点
    auto object_scale_node = canvas_layout_node.child("object-scales");
    if (!object_scale_node)
        object_scale_node = canvas_layout_node.append_child("object-scales");
    auto object_wscale_attrib = object_scale_node.attribute("width");
    auto object_hscale_attrib = object_scale_node.attribute("height");
    if (!object_wscale_attrib) {
        object_wscale_attrib = object_scale_node.append_attribute("width");
        object_wscale_attrib = 1.f;
    }
    if (!object_hscale_attrib) {
        object_hscale_attrib = object_scale_node.append_attribute("height");
        object_hscale_attrib = 1.f;
    }
    if (from_config) {
        object_wscale_attrib = project_config.canvas_config.object_width_scale;
        object_hscale_attrib = project_config.canvas_config.object_height_scale;
    } else {
        project_config.canvas_config.object_width_scale =
            object_wscale_attrib.as_float();
        project_config.canvas_config.object_height_scale =
            object_hscale_attrib.as_float();
    }
}

// 关闭项目
void MProject::close() {
    if (is_closed) return;
    // 通知画布卸载纹理
    texcallback->need_unloadtexture_dir(project_path.generic_string());
    // 通知音频池卸载音轨
    is_closed.store(true);
    // 关闭所有的谱面配置ui
    for (auto& [name, map] : project_maps_table) {
        map->closeConfigui();
    }
    // 写出项目配置文件
    // 根据当前配置更新配置文档
    update_configdoc(true);
    // 保存到文件
    auto configdoc_filepath =
        (project_path / (project_config.project_name + ".mproject"))
            .generic_string();
    config_doc.save_file(configdoc_filepath.c_str());
}
