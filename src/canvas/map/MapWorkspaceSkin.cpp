#include "MapWorkspaceSkin.h"

#include <qlogging.h>

#include <QDir>
#include <filesystem>
#include <fstream>
#include <string>

#include "MapWorkspaceCanvas.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"

MapWorkspaceSkin::MapWorkspaceSkin(MapWorkspaceCanvas* canvas) : cvs(canvas) {}

MapWorkspaceSkin::~MapWorkspaceSkin() = default;

// 统计目录下的文件数
int count_files_recursive(const std::filesystem::path& dir_path) {
    int count = 0;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            count++;
        }
    }
    return count;
}

// 载入皮肤
void MapWorkspaceSkin::load_skin(const std::filesystem::path& skin_path) {
    // 读取配置文件
    auto config_path = skin_path / "skinconfig.json";
    if (!std::filesystem::exists(config_path)) {
        XERROR("皮肤配置文件缺失,请检查[" + config_path.string() + "]");
        return;
    }

    // 解析json
    std::ifstream configfs(config_path);
    configfs >> skin_config;

    // 载入纹理
    cvs->load_texture_from_path(skin_path);

    // XINFO("config content:\n" + skin_config.dump(2));

    // 读取配置
    skinname = skin_config.value<std::string>("name", "unknown");
    authorname = skin_config.value<std::string>("author", "unknown author");

    texture_config = skin_config["textures"];

    bg_texture_config = texture_config["bg"];

    object_texture_config = texture_config["hitobject"];

    note_texture_config = object_texture_config["note"];

    head_texture_config = object_texture_config["head"];

    node_texture_config = object_texture_config["node"];

    hold_texture_config = object_texture_config["hold"];

    vertical_holdbody_texture_config = hold_texture_config["body"]["vertical"];

    horizontal_holdbody_texture_config =
        hold_texture_config["body"]["horizontal"];

    hold_end_texture_config = hold_texture_config["end"];

    left_slide_end_texture_config = object_texture_config["slide"]["arrowleft"];

    right_slide_end_texture_config =
        object_texture_config["slide"]["arrowright"];

    // 音效配置
    auto sound_effects_config = skin_config["sounds"];

    sound_effects[SoundEffectType::COMMON_HIT] =
        QDir((skin_path /
              sound_effects_config.value<std::string>("commonhit", "none")))
            .canonicalPath()
            .toStdString();
    sound_effects[SoundEffectType::SLIDE] =
        QDir((skin_path /
              sound_effects_config.value<std::string>("slidehit", "none")))
            .canonicalPath()
            .toStdString();

    // 载入key音
    BackgroundAudio::loadin_audio(
        QString::fromStdString(
            (skin_path / sound_effects[SoundEffectType::COMMON_HIT]).string())
            .toStdString());
    BackgroundAudio::loadin_audio(
        QString::fromStdString(
            (skin_path / sound_effects[SoundEffectType::SLIDE]).string())
            .toStdString());

    // 选中框纹理配置
    selected_config = bg_texture_config["select-border"];

    // 打击特效配置
    hit_effect_config = texture_config["effects"];
    nomal_hit_effect_dir =
        hit_effect_config.value<std::string>("note-effectdir", "");
    nomal_hit_effect_frame_count =
        count_files_recursive(skin_path / nomal_hit_effect_dir);
    normal_hit_effect_duration =
        hit_effect_config.value<double>("note-effect-duration", 0.1);

    slide_hit_effect_dir =
        hit_effect_config.value<std::string>("slide-effectdir", "");
    slide_hit_effect_frame_count =
        count_files_recursive(skin_path / slide_hit_effect_dir);

    XINFO("ncount:[" + std::to_string(nomal_hit_effect_frame_count) + "]");
    XINFO("scount:[" + std::to_string(slide_hit_effect_frame_count) + "]");

    // 颜色配置
    preview_area_bg_color = QColor::fromString(skin_config.value<std::string>(
        "previewarea-background-color", "#000000FF"));

    // 字体配置
    auto font_config = skin_config["font"];

    font_family = font_config.value<std::string>("font-family", "Noto Sans");
    // XERROR("family:" + font_family);

    timeinfo_font_color = QColor::fromString(
        font_config.value<std::string>("timeinfo-font-color", "#000000FF"));
    timeinfo_font_size = font_config.value<int32_t>("timeinfo-font-size", 16);

    // 读取分拍线主题配置
    auto divisor_config = skin_config["divisortheme"]["divisor-info"];
    for (const auto& [key, value] : divisor_config.items()) {
        auto divs = std::stoi(key);
        auto& infos = divisors_color_theme[divs];
        for (const auto& info : value) {
            // XINFO("info:" + info.dump(2));
            info.value<std::string>("color", "#888888FF");
            infos.emplace_back(QString::fromStdString(info.value<std::string>(
                                   "color", "#888888FF")),
                               info.value<int32_t>("width", 2));
        }
    }
    // XINFO("---------读取结果------------");
    // for (const auto& [divs, pair] : divisors_color_theme) {
    //   XINFO(std::to_string(divs) + "分");
    //   for (const auto& [color, width] : pair) {
    //     qDebug() << "w:" << width << ",color:" << color;
    //   }
    // }
}

// 获取音效
std::string& MapWorkspaceSkin::get_sound_effect(SoundEffectType type) {
    return sound_effects[type];
}

// 获取轨道底板纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_orbit_bg_texture() {
    auto obg_texit = bg_texture_buffer.find(TexType::ORBIT_BG);
    if (obg_texit == bg_texture_buffer.end()) {
        obg_texit =
            bg_texture_buffer
                .try_emplace(
                    TexType::ORBIT_BG,
                    cvs->texture_full_map[bg_texture_config["panel"]
                                              .value<std::string>(
                                                  "orbit_background", "none")])
                .first;
    }
    return obg_texit->second;
}

// 获取轨道判定纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_orbit_judge_texture() {
    auto ojudge_texit = bg_texture_buffer.find(TexType::JUDGE_ORBIT);
    if (ojudge_texit == bg_texture_buffer.end()) {
        ojudge_texit =
            bg_texture_buffer
                .try_emplace(
                    TexType::JUDGE_ORBIT,
                    cvs->texture_full_map[bg_texture_config["panel"]
                                              .value<std::string>("judge_orbit",
                                                                  "none")])
                .first;
    }
    return ojudge_texit->second;
}

// 获取背景的纹理
std::shared_ptr<TextureInstace>&
MapWorkspaceSkin::get_skin_background_texture() {
    auto bg_texit = bg_texture_buffer.find(TexType::BACKGROUND);
    if (bg_texit == bg_texture_buffer.end()) {
        bg_texit =
            bg_texture_buffer
                .try_emplace(
                    TexType::BACKGROUND,
                    cvs->texture_full_map[bg_texture_config.value<std::string>(
                        "background", "none")])
                .first;
    }
    return bg_texit->second;
}

// 获取精灵的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_sprite_texture() {
    return cvs->texture_full_map[bg_texture_config.value<std::string>("sprite",
                                                                      "none")];
}

// 获取判定线的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_judgeline_texture() {
    return cvs->texture_full_map[bg_texture_config.value<std::string>(
        "judgeline", "none")];
}

// 获取选择框纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_selected_border_texture(
    SelectBorderDirection direction) {
    switch (direction) {
        case SelectBorderDirection::LEFT: {
            return cvs->texture_full_map[selected_config.value<std::string>(
                "left", "none")];
            break;
        }
        case SelectBorderDirection::RIGHT: {
            return cvs->texture_full_map[selected_config.value<std::string>(
                "right", "none")];
            break;
        }
        case SelectBorderDirection::TOP: {
            return cvs->texture_full_map[selected_config.value<std::string>(
                "top", "none")];
            break;
        }
        case SelectBorderDirection::BOTTOM: {
            return cvs->texture_full_map[selected_config.value<std::string>(
                "bottom", "none")];
            break;
        }
        default:
            return cvs->texture_full_map["none"];
    }
}

// 获取物件的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_object_texture(
    TexType type, ObjectStatus status) {
    // TODO(xiang 2025-05-07): 优化性能-缓存json结果防止一直读取json
    json* config = nullptr;
    std::string key;

    // 检查一层缓存
    auto status_map_it = object_texture_buffer.find(type);
    if (status_map_it == object_texture_buffer.end()) {
        // 无一层缓存-创建对应一层缓存表
        status_map_it = object_texture_buffer.try_emplace(type).first;
    } else {
        // 直接读取一层缓存检查二层缓存表
        auto texture_it = status_map_it->second.find(status);
        if (texture_it == status_map_it->second.end()) {
            // 无一层缓存对应的二层缓存表-创建对应二层缓存表
            texture_it = status_map_it->second.try_emplace(status).first;
        } else {
            // 直接读取结果
            return texture_it->second;
        }
    }
    switch (type) {
        case TexType::NORMAL_NOTE: {
            config = &note_texture_config;
            break;
        }
        case TexType::HOLD_HEAD: {
            config = &head_texture_config;
            break;
        }
        case TexType::NODE: {
            config = &node_texture_config;
            break;
        }
        case TexType::HOLD_BODY_VERTICAL: {
            config = &vertical_holdbody_texture_config;
            break;
        }
        case TexType::HOLD_BODY_HORIZONTAL: {
            config = &horizontal_holdbody_texture_config;
            break;
        }
        case TexType::HOLD_END: {
            config = &hold_end_texture_config;
            break;
        }
        case TexType::SLIDE_END_LEFT: {
            config = &left_slide_end_texture_config;
            break;
        }
        case TexType::SLIDE_END_RIGHT: {
            config = &right_slide_end_texture_config;
            break;
        }
    }

    switch (status) {
        case ObjectStatus::COMMON: {
            key = "common";
            break;
        }
        case ObjectStatus::HOVER: {
            key = "hover";
            break;
        }
        case ObjectStatus::SELECTED: {
            key = "selected";
            break;
        }
    }
    // 填入缓存并返回
    auto texture =
        cvs->texture_full_map[config->value<std::string>(key, "unknown")];
    object_texture_buffer[type][status] = texture;
    return object_texture_buffer[type][status];
}
