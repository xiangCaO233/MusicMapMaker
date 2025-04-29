#include "MapWorkspaceSkin.h"

#include <qlogging.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "MapWorkspaceCanvas.h"
#include "colorful-log.h"

MapWorkspaceSkin::MapWorkspaceSkin(MapWorkspaceCanvas* canvas) : cvs(canvas) {}

MapWorkspaceSkin::~MapWorkspaceSkin() = default;

// 载入皮肤
void MapWorkspaceSkin::load_skin(std::filesystem::path& skin_path) {
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

  head_texture_config = object_texture_config["head"];

  node_texture_config = object_texture_config["node"];

  hold_texture_config = object_texture_config["hold"];

  vertical_holdbody_texture_config = hold_texture_config["body"]["vertical"];

  horizontal_holdbody_texture_config =
      hold_texture_config["body"]["horizontal"];

  hold_end_texture_config = hold_texture_config["end"];

  left_slide_end_texture_config = object_texture_config["slide"]["arrowleft"];

  right_slide_end_texture_config = object_texture_config["slide"]["arrowright"];

  // 颜色配置
  preview_area_bg_color = QColor::fromString(skin_config.value<std::string>(
      "previewarea-background-color", "#000000FF"));

  // 字体配置
  auto font_config = skin_config["font"];
  font_family = font_config.value<std::string>("font-family", "Noto Sans");
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
      infos.emplace_back(
          QString::fromStdString(info.value<std::string>("color", "#888888FF")),
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

// 获取背景的纹理
std::shared_ptr<TextureInstace>&
MapWorkspaceSkin::get_skin_background_texture() {
  return cvs->texture_full_map[bg_texture_config.value<std::string>(
      "background", "none")];
}

// 获取精灵的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_sprite_texture() {
  return cvs->texture_full_map[bg_texture_config.value<std::string>("sprite",
                                                                    "none")];
}

// 获取判定线的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_judgeline_texture() {
  return cvs->texture_full_map[bg_texture_config.value<std::string>("judgeline",
                                                                    "none")];
}

// 获取物件的纹理
std::shared_ptr<TextureInstace>& MapWorkspaceSkin::get_object_texture(
    TexType type, ObjectStatus status) {
  json* config;
  std::string key;

  switch (type) {
    case TexType::NOTE_HEAD: {
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
  return cvs->texture_full_map[config->value<std::string>(key, "unknown")];
}
