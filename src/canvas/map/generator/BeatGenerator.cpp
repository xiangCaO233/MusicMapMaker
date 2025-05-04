#include "BeatGenerator.h"

#include <utility>

#include "../../../util/mutil.h"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"

// 节拍线渲染数据队列(确定层级)
std::queue<LineRenderData> BeatGenerator::line_queue;

// 节拍线渲染数据队列(确定层级)
std::queue<TimeTextRenderData> BeatGenerator::text_queue;

// 构造BeatGenerator
BeatGenerator::BeatGenerator(std::shared_ptr<MapEditor> editor)
    : editor_ref(editor) {}

// 析构BeatGenerator
BeatGenerator::~BeatGenerator() = default;

// 生成拍渲染指令
void BeatGenerator::generate() {
  for (int i = 0; i < editor_ref->ebuffer.current_beats.size(); ++i) {
    auto &beat = editor_ref->ebuffer.current_beats[i];

    // 每拍时间*时间线缩放=拍距
    double beat_distance =
        60.0 / beat->bpm * 1000.0 * editor_ref->cstatus.timeline_zoom *
        (editor_ref->cstatus.canvas_pasued ? 1.0
                                           : editor_ref->cstatus.speed_zoom);

    // 分拍间距
    double divisor_distance = beat_distance / beat->divisors;

    // 判定线位置
    auto judgeline_pos = editor_ref->cstatus.canvas_size.height() *
                         (1.0 - editor_ref->csettings.judgeline_position);

    // 拍起始时间
    auto &beat_start_time = beat->start_timestamp;

    // 拍距离判定线距离从下往上--反转
    // 当前拍起始位置
    auto beat_start_pos =
        judgeline_pos -
        (beat_start_time - editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->cstatus.timeline_zoom *
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
  }
}
