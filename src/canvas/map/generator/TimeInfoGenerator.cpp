#include "TimeInfoGenerator.h"

#include <QString>
#include <algorithm>
#include <memory>

#include "../../../util/mutil.h"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"

// 构造TimeInfoGenerator
TimeInfoGenerator::TimeInfoGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {}

// 析构TimeInfoGenerator
TimeInfoGenerator::~TimeInfoGenerator() = default;

// 生成信息
void TimeInfoGenerator::generate() {
  // 分隔线
  editor_ref->canvas_ref->renderer_manager->addLine(
      QPointF(editor_ref->cstatus.canvas_size.width() *
                      editor_ref->csettings.infoarea_width_scale -
                  2,
              0),
      QPointF(editor_ref->cstatus.canvas_size.width() *
                      editor_ref->csettings.infoarea_width_scale -
                  2,
              editor_ref->cstatus.canvas_size.height()),
      4, nullptr, QColor(255, 182, 193, 235), false);

  // 标记timing
  draw_timing_points();
}

// 绘制时间点
void TimeInfoGenerator::draw_timing_points() {
  if (!editor_ref->canvas_ref->working_map) return;

  // 判定线位置
  auto judgeline_pos = editor_ref->cstatus.canvas_size.height() *
                       (1.0 - editor_ref->csettings.judgeline_position);

  std::string bpm_prefix = "bpm:";
  std::vector<std::vector<std::shared_ptr<Timing>> *> timingss_in_area;
  std::vector<std::shared_ptr<Timing>> timings_in_area;

  QSize connectionSize(12, 20);

  // 查询区间内的timing
  editor_ref->canvas_ref->working_map->query_timing_in_range(
      timings_in_area, editor_ref->ebuffer.current_time_area_start,
      editor_ref->ebuffer.current_time_area_end);

  editor_ref->canvas_ref->working_map->query_timing_in_range(
      timingss_in_area, editor_ref->ebuffer.current_time_area_start,
      editor_ref->ebuffer.current_time_area_end);

  for (const auto &timings : timingss_in_area) {
    // 实际在此时间点显示的timing的bpm值
    double display_bpm_value{0};
    // 是否显示变速值
    bool show_speed{false};
    // 若显示变速值参考的timing
    std::shared_ptr<Timing> speed_timing_ref{nullptr};

    // 根据此同时间戳timing表大小确定是否存在即时变速
    // 预处理同时间戳timing表
    if (timings->size() >= 2) {
      // 包含同时间点timing
      if (timings->at(0)->basebpm == timings->at(1)->basebpm) {
        display_bpm_value = timings->at(0)->basebpm;
      }

      // 检查是否存在变速timing
      for (const auto &timing : *timings) {
        if (!timing->is_base_timing) {
          show_speed = true;
          speed_timing_ref = timing;
        }
      }
    } else {
      // 单独timing
      display_bpm_value = timings->at(0)->basebpm;
      if (!timings->at(0)->is_base_timing) {
        show_speed = true;
        speed_timing_ref = timings->at(0);
      }
    }

    // 格式化显示的字符串参考-bpm
    std::string preference_bpm_str =
        "bpm:" + QString::number(display_bpm_value, 'f', 2).toStdString();
    std::string speed_str = "";

    if (show_speed) {
      speed_str = "speed:" +
                  QString::number(speed_timing_ref->bpm, 'f', 2).toStdString();
    }

#ifdef _WIN32
#else
    std::u32string u32absbpmstr = mutil::cu32(preference_bpm_str);
    std::u32string u32speedstr = mutil::cu32(speed_str);
#endif  //_WIN32

    // 计算字符串metrics
    double prebpmstr_width{0};
    double prebpmstr_height{0};

    // 参考bpm
    for (const auto &character : u32absbpmstr) {
      auto charsize =
          editor_ref->canvas_ref->renderer_manager->get_character_size(
              editor_ref->canvas_ref->skin.font_family,
              editor_ref->canvas_ref->skin.timeinfo_font_size, character);
      prebpmstr_width += charsize.width();

      if (charsize.height() > prebpmstr_height) {
        prebpmstr_height = charsize.height();
      }
    }

    double speedstr_width{0};
    double speedstr_height{0};
    // 速度
    for (const auto &character : u32speedstr) {
      auto charsize =
          editor_ref->canvas_ref->renderer_manager->get_character_size(
              editor_ref->canvas_ref->skin.font_family,
              editor_ref->canvas_ref->skin.timeinfo_font_size, character);
      speedstr_width += charsize.width();

      if (charsize.height() > speedstr_height) {
        speedstr_height = charsize.height();
      }
    }

    // 全部字符串的尺寸
    QSizeF strs_size(std::max(prebpmstr_width, speedstr_width),
                     prebpmstr_height + speedstr_height);

    // 内外框的尺寸
    QSizeF inner_bound_size(
        strs_size.width() + editor_ref->canvas_ref->skin.timeinfo_font_size,
        strs_size.height() + editor_ref->canvas_ref->skin.timeinfo_font_size);
  }

  for (const auto &timing : timings_in_area) {
    auto timing_y_pos =
        judgeline_pos -
        (timing->timestamp - editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->cstatus.timeline_zoom *
            (editor_ref->cstatus.canvas_pasued
                 ? 1.0
                 : editor_ref->cstatus.speed_zoom);
    double timing_x_pos;

    std::string bpmvalue =
        QString::number(timing->basebpm, 'f', 2).toStdString();

    // 绝对bpm-参考
    auto absbpm_info = bpm_prefix + bpmvalue;

#ifdef _WIN32
#else
    std::u32string absbpm = mutil::cu32(absbpm_info);
#endif  //_WIN32
    // 计算bpm字符串尺寸
    auto absbpm_info_string_width{0};
    auto absbpm_info_string_height{0};
    for (const auto &character : absbpm) {
      auto charsize =
          editor_ref->canvas_ref->renderer_manager->get_character_size(
              editor_ref->canvas_ref->skin.font_family,
              editor_ref->canvas_ref->skin.timeinfo_font_size, character);
      absbpm_info_string_width += charsize.width();

      if (charsize.height() > absbpm_info_string_height) {
        absbpm_info_string_height = charsize.height();
      }
    }
    timing_x_pos =
        editor_ref->ebuffer.edit_area_start_pos_x - absbpm_info_string_width;

    if (!timing->is_base_timing) {
      std::string speed_prefix = tr("speed:").toStdString();
      std::string speedvalue =
          QString::number(timing->bpm, 'f', 2).toStdString();
      auto speed_info = speed_prefix + speedvalue;
#ifdef _WIN32
#else
      std::u32string speed = mutil::cu32(speed_info);
#endif  //_WIN32
      // 计算变速字符串尺寸
      auto speed_info_string_width{0};
      auto speed_info_string_height{0};
      for (const auto &character : speed) {
        auto charsize =
            editor_ref->canvas_ref->renderer_manager->get_character_size(
                editor_ref->canvas_ref->skin.font_family,
                editor_ref->canvas_ref->skin.timeinfo_font_size, character);
        speed_info_string_width += charsize.width();
        if (charsize.height() > speed_info_string_height) {
          speed_info_string_height = charsize.height();
        }
      }
      if (editor_ref->ebuffer.edit_area_start_pos_x - speed_info_string_width <
          timing_x_pos)
        timing_x_pos =
            editor_ref->ebuffer.edit_area_start_pos_x - speed_info_string_width;
      if (std::fabs(timing->bpm * timing->basebpm /
                        editor_ref->canvas_ref->working_map->preference_bpm -
                    1.0) < 0.01) {
      } else {
        // 先画绝对bpm
        editor_ref->canvas_ref->renderer_manager->addText(
            {timing_x_pos - editor_ref->canvas_ref->skin.timeinfo_font_size / 2,
             timing_y_pos - speed_info_string_height -
                 editor_ref->canvas_ref->skin.timeinfo_font_size / 2 -
                 connectionSize.height()},
            absbpm, editor_ref->canvas_ref->skin.timeinfo_font_size,
            editor_ref->canvas_ref->skin.font_family,
            editor_ref->canvas_ref->skin.timeinfo_font_color, 0);
        // 再画变速bpm
        editor_ref->canvas_ref->renderer_manager->addText(
            {timing_x_pos - editor_ref->canvas_ref->skin.timeinfo_font_size / 2,
             timing_y_pos -
                 editor_ref->canvas_ref->skin.timeinfo_font_size / 2 -
                 connectionSize.height()},
            speed, editor_ref->canvas_ref->skin.timeinfo_font_size,
            editor_ref->canvas_ref->skin.font_family,
            editor_ref->canvas_ref->skin.timeinfo_font_color, 0);
      }
    }

    if (timing->is_base_timing) {
      // 只画绝对bpm
      editor_ref->canvas_ref->renderer_manager->addText(
          {timing_x_pos - editor_ref->canvas_ref->skin.timeinfo_font_size / 2,
           timing_y_pos - editor_ref->canvas_ref->skin.timeinfo_font_size / 2 -
               connectionSize.height()},
          absbpm, editor_ref->canvas_ref->skin.timeinfo_font_size,
          editor_ref->canvas_ref->skin.font_family,
          editor_ref->canvas_ref->skin.timeinfo_font_color, 0);
    }
  }
}
