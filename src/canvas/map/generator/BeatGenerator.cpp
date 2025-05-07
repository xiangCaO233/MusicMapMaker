#include "BeatGenerator.h"

#include <utility>

#include "../../../mmm/MapWorkProject.h"
#include "../../../util/mutil.h"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"

// 节拍线渲染数据队列(确定层级)
std::queue<LineRenderData> BeatGenerator::line_queue;

// 节拍线渲染数据队列(确定层级)
std::queue<TimeTextRenderData> BeatGenerator::text_queue;

// 分拍背景渲染数据队列(确定层级)
std::queue<QRectF> BeatGenerator::divbg_queue;

// 构造BeatGenerator
BeatGenerator::BeatGenerator(std::shared_ptr<MapEditor> editor)
    : editor_ref(editor) {}

// 析构BeatGenerator
BeatGenerator::~BeatGenerator() = default;

// 生成拍渲染指令
void BeatGenerator::generate() {
  for (int i = 0; i < editor_ref->ebuffer.current_beats.size(); ++i) {
    auto &beat = editor_ref->ebuffer.current_beats[i];
    if (!beat->divisors_customed) {
      // 未单独调节过分拍策略或是读取的
      // 应用默认分拍策略
      beat->divisors = editor_ref->canvas_ref->working_map->project_reference
                           ->config.default_divisors;
    }

    // 每拍时间*时间线缩放=拍距
    double beat_distance =
        60.0 / beat->bpm * 1000.0 *
        editor_ref->canvas_ref->working_map->project_reference->config
            .timeline_zoom *
        (editor_ref->cstatus.canvas_pasued ? 1.0
                                           : editor_ref->cstatus.speed_zoom);

    // 分拍间距
    double divisor_distance = beat_distance / beat->divisors;

    // 拍起始时间
    auto &beat_start_time = beat->start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        editor_ref->ebuffer.judgeline_position -
        (beat_start_time - editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->canvas_ref->working_map->project_reference->config
                .timeline_zoom *
            (editor_ref->cstatus.canvas_pasued
                 ? 1.0
                 : editor_ref->cstatus.speed_zoom);

    // 绘制小节线
    for (int j = 0; j < beat->divisors; ++j) {
      auto divisor_time =
          (beat->start_timestamp +
           (beat->end_timestamp - beat_start_time) / beat->divisors * j);
      // 筛选越界分拍线
      if (i < editor_ref->ebuffer.current_beats.size() - 1) {
        // 并非这波最后一拍
        // 取出下一拍引用
        auto &nextbeat = editor_ref->ebuffer.current_beats[i + 1];
        if (divisor_time > nextbeat->start_timestamp) {
          // 这个分拍线超过了下一拍的起始时间--跳过此分拍线的绘制
          continue;
        }
      }

      // 小节线的位置
      double divisor_pos = beat_start_pos - j * divisor_distance;
      if (divisor_pos >= -beat_distance &&
          divisor_pos <=
              editor_ref->cstatus.canvas_size.height() + beat_distance) {
        // 只绘制在可视范围内的小节线
        // 过滤abstiming之前的小节线
        if (divisor_time >= editor_ref->ebuffer.current_abs_timing->timestamp) {
          if (j == 0) {
            // 添加小节线头,粗一点6px
            // 固定使用白色
            /*
             *struct LineRenderData {
             *  double x1;
             *  double y1;
             *  double x2;
             *  double y2;
             *
             *  float r;
             *  float g;
             *  float b;
             *  float a;
             *  int32_t line_width;
             *};
             */
            line_queue.emplace(
                0, divisor_pos,
                editor_ref->cstatus.canvas_size.width() *
                    (1 - editor_ref->csettings.preview_width_scale),
                divisor_pos, 255, 255, 255, 255, 8);
          } else {
            auto divinfos = editor_ref->canvas_ref->skin
                                .divisors_color_theme[beat->divisors];
            std::pair<QColor, int32_t> divinfo;
            if (divinfos.empty()) {
              divinfo =
                  std::pair<QColor, int32_t>(QColor(128, 128, 128, 255), 2);
            } else {
              divinfo = divinfos[j - 1];
            }
            line_queue.emplace(
                0, divisor_pos,
                editor_ref->cstatus.canvas_size.width() *
                    (1 - editor_ref->csettings.preview_width_scale),
                divisor_pos, divinfo.first.red(), divinfo.first.green(),
                divinfo.first.blue(), divinfo.first.alpha(), divinfo.second);
          }

          // 计算精确时间--格式化
          std::u32string timestr;
          mutil::format_music_time2u32(timestr, divisor_time);
          // 添加精确时间到时间字符串绘制队列
          /*
           *struct TimeTextRenderData {
           *  double x;
           *  double y;
           *
           *  std::u32string text;
           *};
           */
          text_queue.emplace(4, divisor_pos - 4, std::move(timestr));
        }
      }
    }
    // 分拍策略-绘制到此拍中间位置
    std::u32string divinfo =
        QString("1/%1").arg(beat->divisors).toStdU32String();
    // 计算字符串metrics
    double divstr_width{0};
    double divstr_height{0};
    for (const auto &character : divinfo) {
      auto charsize =
          editor_ref->canvas_ref->renderer_manager->get_character_size(
              editor_ref->canvas_ref->skin.font_family,
              editor_ref->canvas_ref->skin.timeinfo_font_size, character);
      divstr_width += charsize.width();

      if (charsize.height() > divstr_height) {
        divstr_height = charsize.height();
      }
    }
    // 框的尺寸
    QSizeF bound_size(
        divstr_width + editor_ref->canvas_ref->skin.timeinfo_font_size,
        divstr_height + editor_ref->canvas_ref->skin.timeinfo_font_size);
    QRectF str_bound(
        (editor_ref->cstatus.info_area.width() - bound_size.width()) / 2.0,
        beat_start_pos - (beat_distance - bound_size.height()) / 2.0 -
            bound_size.height(),
        bound_size.width(), bound_size.height());
    // 添加绘制分拍信息框到队列
    divbg_queue.emplace(str_bound);
    // editor_ref->canvas_ref->renderer_manager->addRoundRect(
    //     str_bound, nullptr, QColor(64, 64, 64, 200), 0, 1.1, true);

    // 添加字符串绘制到队列
    text_queue.emplace(
        str_bound.x() + editor_ref->canvas_ref->skin.timeinfo_font_size / 2.0,
        str_bound.y() + editor_ref->canvas_ref->skin.timeinfo_font_size / 2.0 +
            divstr_height,
        divinfo);
  }
}
