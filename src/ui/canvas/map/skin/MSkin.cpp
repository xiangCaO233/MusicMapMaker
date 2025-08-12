#include <QDebug>
#include <QDir>
#include <fstream>
#include <map/skin/MSkin.hpp>

// 构造MSkin
MSkin::MSkin(std::string_view skin_path, TextureLoadCallback* textureLoadcbk,
             AudioLoadCallback* audioLoadcbk) {
    // 载入皮肤配置
    skinPath = std::filesystem::path(skin_path);
    std::ifstream ifs(skinPath / "skinconfig.json");
    ifs >> skinRootCfg;

    // 读取配置
    name = skinRootCfg.value<std::string>("name", "unknown");
    author = skinRootCfg.value<std::string>("author", "unknown author");
    auto texture_config = skinRootCfg["textures"];

    auto bg_texture_config = texture_config["bg"];

    auto object_texture_config = texture_config["hitobject"];

    auto note_texture_config = object_texture_config["note"];

    auto head_texture_config = object_texture_config["head"];

    auto node_texture_config = object_texture_config["node"];

    auto hold_texture_config = object_texture_config["hold"];

    auto vertical_holdbody_texture_config =
        hold_texture_config["body"]["vertical"];

    auto horizontal_holdbody_texture_config =
        hold_texture_config["body"]["horizontal"];

    auto hold_end_texture_config = hold_texture_config["end"];

    auto left_slide_end_texture_config =
        object_texture_config["slide"]["arrowleft"];

    auto right_slide_end_texture_config =
        object_texture_config["slide"]["arrowright"];

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

    // 载入皮肤音轨
    audioLoadcbk->loadBack(sound_effects[SoundEffectType::COMMON_HIT]);
    audioLoadcbk->loadBack(sound_effects[SoundEffectType::SLIDE]);

    qDebug() << "载入皮肤:[" << name << "]";
    qDebug() << "皮肤作者:[" << author << "]";
}

// 析构MSkin
MSkin::~MSkin() {}

// 皮肤基本字体族
std::string_view MSkin::fontFamilyASCII() {}

// ascii无字符时使用
std::string_view MSkin::fontFamilyUTF8() {}

// 获取轨道判定纹理
std::string_view MSkin::get_orbit_judge_texture() {}

// 获取轨道底板纹理
std::string_view MSkin::get_orbit_bg_texture() {}

// 获取选择框纹理
std::string_view MSkin::get_selected_border_texture(
    SelectBorderDirection direction) {}

// 获取判定线的纹理
std::string_view MSkin::get_judgeline_texture() {}

// 获取物件的纹理
std::string_view MSkin::get_object_texture(TexType type, ObjectStatus status) {}

// 获取音效
std::string_view MSkin::get_sound_effect(SoundEffectType type) {}
