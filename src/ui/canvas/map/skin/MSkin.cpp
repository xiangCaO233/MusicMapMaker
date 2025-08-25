#include <QDebug>
#include <QDir>
#include <fstream>
#include <map/skin/MSkin.hpp>

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

// 构造MSkin
MSkin::MSkin(std::string_view skin_path, AudioLoadCallback* audioLoadcbk,
             TextureLoadCallback* textureLoadcbk)
    : texcallback(textureLoadcbk) {
    // 载入皮肤配置
    skinPath = std::filesystem::path(skin_path);
    auto cfgPath = skinPath / "skinconfig.json";
    std::ifstream ifs(cfgPath);
    qDebug() << "配置路径:" << cfgPath;
    ifs >> skinRootCfg;

    // 读取配置
    name = skinRootCfg.value<std::string>("name", "unknown");
    author = skinRootCfg.value<std::string>("author", "unknown author");

    texture_config = skinRootCfg["textures"];
    qDebug() << "texture_config :" << texture_config.dump(4);

    bg_texture_config = texture_config["bg"];
    qDebug() << "bg_texture_config :" << bg_texture_config.dump(4);

    object_texture_config = texture_config["hitobject"];
    qDebug() << "object_texture_config:" << object_texture_config.dump(4);

    note_texture_config = object_texture_config["note"];
    qDebug() << "note_texture_config:" << note_texture_config.dump(4);

    head_texture_config = object_texture_config["head"];
    qDebug() << "head_texture_config:" << head_texture_config.dump(4);

    node_texture_config = object_texture_config["node"];
    qDebug() << "node_texture_config:" << node_texture_config.dump(4);

    hold_texture_config = object_texture_config["hold"];
    qDebug() << "hold_texture_config :" << hold_texture_config.dump(4);

    vertical_holdbody_texture_config = hold_texture_config["body"]["vertical"];
    qDebug() << "vertical_holdbody_texture_config :"
             << vertical_holdbody_texture_config.dump(4);

    horizontal_holdbody_texture_config =
        hold_texture_config["body"]["horizontal"];
    qDebug() << "horizontal_holdbody_texture_config :"
             << horizontal_holdbody_texture_config.dump(4);

    hold_end_texture_config = hold_texture_config["end"];
    qDebug() << "hold_end_texture_config :" << hold_end_texture_config.dump(4);

    left_slide_end_texture_config = object_texture_config["slide"]["arrowleft"];
    qDebug() << "left_slide_end_texture_config :"
             << left_slide_end_texture_config.dump(4);

    right_slide_end_texture_config =
        object_texture_config["slide"]["arrowright"];
    qDebug() << "right_slide_end_texture_config :"
             << right_slide_end_texture_config.dump(4);

    // 音效配置
    auto sound_effects_config = skinRootCfg["sounds"];

    sound_effects[SoundEffectType::COMMON_HIT] =
        QDir((skinPath /
              sound_effects_config.value<std::string>("commonhit", "none")))
            .canonicalPath()
            .toStdString();
    sound_effects[SoundEffectType::SLIDE] =
        QDir((skinPath /
              sound_effects_config.value<std::string>("slidehit", "none")))
            .canonicalPath()
            .toStdString();

    // 选中框纹理配置
    selected_config = bg_texture_config["select-border"];
    qDebug() << "selected_config :" << selected_config.dump(4);

    // 打击特效配置
    hit_effect_config = texture_config["effects"];
    qDebug() << "hit_effect_config :" << hit_effect_config.dump(4);
    nomal_hit_effect_dir =
        hit_effect_config.value<std::string>("note-effectdir", "");
    nomal_hit_effect_frame_count = count_files_recursive(
        std::filesystem::path(skin_path) / nomal_hit_effect_dir);
    normal_hit_effect_duration =
        hit_effect_config.value<double>("note-effect-duration", 0.1);

    slide_hit_effect_dir =
        hit_effect_config.value<std::string>("slide-effectdir", "");
    slide_hit_effect_frame_count = count_files_recursive(
        std::filesystem::path(skin_path) / slide_hit_effect_dir);

    // 颜色配置
    preview_area_bg_color = QColor::fromString(skinRootCfg.value<std::string>(
        "previewarea-background-color", "#000000FF"));

    // 字体配置
    auto font_config = skinRootCfg["font"];

    fontascii = font_config.value<std::string>("font-family", "Noto Sans");

    timeinfo_font_color = QColor::fromString(
        font_config.value<std::string>("timeinfo-font-color", "#000000FF"));
    timeinfo_font_size = font_config.value<int32_t>("timeinfo-font-size", 16);

    // 读取分拍线主题配置
    auto divisor_config = skinRootCfg["divisortheme"]["divisor-info"];
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

    // 载入默认皮肤的全部纹理
    textureLoadcbk->need_loadtexture_dir(skin_path);

    qDebug() << "载入皮肤:[" << name << "]";
    qDebug() << "皮肤作者:[" << author << "]";
}

// 析构MSkin
MSkin::~MSkin() {}

// 皮肤基本字体族
std::string_view MSkin::fontFamilyASCII() { return fontascii; }

// ascii无字符时使用
std::string_view MSkin::fontFamilyUTF8() { return fontu8; }

// 获取轨道判定纹理
TextureInfo MSkin::get_orbit_judge_texture() {
    auto ojudge_texit = bg_texture_buffer.find(TexType::JUDGE_ORBIT);
    if (ojudge_texit == bg_texture_buffer.end()) {
        ojudge_texit =
            bg_texture_buffer
                .try_emplace(TexType::JUDGE_ORBIT,
                             texcallback->getInfo(
                                 bg_texture_config["panel"].value<std::string>(
                                     "judge_orbit", "none")))
                .first;
    }
    return ojudge_texit->second;
}

// 获取轨道底板纹理
TextureInfo MSkin::get_orbit_bg_texture() {
    auto obg_texit = bg_texture_buffer.find(TexType::ORBIT_BG);
    if (obg_texit == bg_texture_buffer.end()) {
        obg_texit =
            bg_texture_buffer
                .try_emplace(TexType::ORBIT_BG,
                             texcallback->getInfo(
                                 bg_texture_config["panel"].value<std::string>(
                                     "orbit_background", "none")))
                .first;
    }
    return obg_texit->second;
}

// 获取选择框纹理
TextureInfo MSkin::get_selected_border_texture(
    SelectBorderDirection direction) {
    switch (direction) {
        case SelectBorderDirection::LEFT: {
            return texcallback->getInfo(
                selected_config.value<std::string>("left", "none"));
            break;
        }
        case SelectBorderDirection::RIGHT: {
            return texcallback->getInfo(
                selected_config.value<std::string>("right", "none"));
            break;
        }
        case SelectBorderDirection::TOP: {
            return texcallback->getInfo(
                selected_config.value<std::string>("top", "none"));
            break;
        }
        case SelectBorderDirection::BOTTOM: {
            return texcallback->getInfo(
                selected_config.value<std::string>("bottom", "none"));
            break;
        }
        default:
            return texcallback->getInfo("none");
    }
}

// 获取判定线的纹理
TextureInfo MSkin::get_judgeline_texture() {
    return texcallback->getInfo(
        bg_texture_config.value<std::string>("judgeline", "none"));
}

// 获取物件的纹理
TextureInfo MSkin::get_object_texture(TexType type, ObjectStatus status) {
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
        using enum TexType;
        case NORMAL_NOTE: {
            config = &note_texture_config;
            break;
        }
        case HOLD_HEAD: {
            config = &head_texture_config;
            break;
        }
        case NODE: {
            config = &node_texture_config;
            break;
        }
        case HOLD_BODY_VERTICAL: {
            config = &vertical_holdbody_texture_config;
            break;
        }
        case HOLD_BODY_HORIZONTAL: {
            config = &horizontal_holdbody_texture_config;
            break;
        }
        case HOLD_END: {
            config = &hold_end_texture_config;
            break;
        }
        case SLIDE_END_LEFT: {
            config = &left_slide_end_texture_config;
            break;
        }
        case SLIDE_END_RIGHT: {
            config = &right_slide_end_texture_config;
            break;
        }
    }

    switch (status) {
        using enum ObjectStatus;
        case COMMON: {
            key = "common";
            break;
        }
        case HOVER: {
            key = "hover";
            break;
        }
        case SELECTED: {
            key = "selected";
            break;
        }
    }
    // 填入缓存并返回
    auto rpath =
        std::filesystem::path(config->value<std::string>(key, "unknown"));
    auto apath = skinPath / rpath;
    auto texture = texcallback->getInfo(apath.generic_string());
    object_texture_buffer[type][status] = texture;
    return object_texture_buffer[type][status];
}

// 获取音效
std::string_view MSkin::get_sound_effect(SoundEffectType type) {
    return sound_effects[type];
}
