#include "TimeInfoGenerator.h"

#include <QString>

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
      QPointF(
          editor_ref->canvas_size.width() * editor_ref->infoarea_width_scale -
              2,
          0),
      QPointF(
          editor_ref->canvas_size.width() * editor_ref->infoarea_width_scale -
              2,
          editor_ref->canvas_size.height()),
      4, nullptr, QColor(255, 182, 193, 235), false);

  // 标记timing
  draw_timing_points();
}

// 绘制时间点
void TimeInfoGenerator::draw_timing_points() {
  if (!editor_ref->canvas_ref->working_map) return;

  std::string bpm_prefix = "bpm:";
  std::vector<std::shared_ptr<Timing>> timings_in_area;

  // 查询区间内的timing
  editor_ref->canvas_ref->working_map->query_timing_in_range(
      timings_in_area, editor_ref->current_time_area_start,
      editor_ref->current_time_area_end);
  QSizeF connectionSize(12, 16);

  // 判定线位置
  auto judgeline_pos =
      editor_ref->canvas_size.height() * (1.0 - editor_ref->judgeline_position);

  for (const auto &timing : timings_in_area) {
    auto timing_y_pos =
        judgeline_pos -
        (timing->timestamp - editor_ref->current_visual_time_stamp) *
            editor_ref->timeline_zoom *
            (editor_ref->canvas_pasued ? 1.0 : editor_ref->speed_zoom);
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
    timing_x_pos = editor_ref->edit_area_start_pos_x - absbpm_info_string_width;

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
      if (editor_ref->edit_area_start_pos_x - speed_info_string_width <
          timing_x_pos)
        timing_x_pos =
            editor_ref->edit_area_start_pos_x - speed_info_string_width;
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
