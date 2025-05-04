#include "AreaInfoGenerator.h"

#include <vector>

#include "../../../mmm/timing/Timing.h"
#include "../../../mmm/timing/osu/OsuTiming.h"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"

// 构造AreaInfoGenerator
AreaInfoGenerator::AreaInfoGenerator(std::shared_ptr<MapEditor> editor)
    : editor_ref(editor) {}

// 析构AreaInfoGenerator
AreaInfoGenerator::~AreaInfoGenerator() = default;

// 生成信息
void AreaInfoGenerator::generate() {
  // 读取获取当前时间附近的timings
  std::vector<std::shared_ptr<Timing>> temp_timings;
  editor_ref->canvas_ref->working_map->query_around_timing(
      temp_timings, editor_ref->cstatus.current_visual_time_stamp);

  // 未查询到timing-不绘制时间线
  if (temp_timings.empty()) return;

  // 更新绝对timing
  for (const auto &timing : temp_timings) {
    switch (timing->type) {
      case TimingType::OSUTIMING: {
        auto otiming = std::static_pointer_cast<OsuTiming>(timing);
        if (!otiming->is_inherit_timing) {
          // 非变速timing--存储的实际bpm
          editor_ref->ebuffer.current_abs_timing = otiming;
          editor_ref->cstatus.speed_zoom =
              editor_ref->ebuffer.current_abs_timing->basebpm /
              editor_ref->canvas_ref->working_map->preference_bpm;
          emit editor_ref->canvas_ref->current_absbpm_changed(
              editor_ref->ebuffer.current_abs_timing->basebpm);
          emit editor_ref->canvas_ref->current_timeline_speed_changed(
              editor_ref->cstatus.speed_zoom);
        } else {
          // 变速timing--存储的倍速
          editor_ref->cstatus.speed_zoom =
              editor_ref->ebuffer.current_abs_timing->basebpm /
              editor_ref->canvas_ref->working_map->preference_bpm *
              otiming->bpm;
          emit editor_ref->canvas_ref->current_timeline_speed_changed(
              editor_ref->cstatus.speed_zoom);
        }
        break;
      }
      case TimingType::RMTIMING: {
        // rmtiming只能存bpm
        auto rmtiming = std::static_pointer_cast<Timing>(timing);
        editor_ref->ebuffer.current_abs_timing = rmtiming;
        editor_ref->cstatus.speed_zoom = 1.0;
        emit editor_ref->canvas_ref->current_absbpm_changed(
            editor_ref->ebuffer.current_abs_timing->basebpm);
        emit editor_ref->canvas_ref->current_timeline_speed_changed(
            editor_ref->cstatus.speed_zoom);
        break;
      }
      case TimingType::MALODYTIMING: {
        break;
      }
      case TimingType::UNKNOWN:
        return;
    }
  }

  // 使用timing计算时间
  // 每拍时间
  auto beattime = 60.0 / editor_ref->ebuffer.current_abs_timing->bpm * 1000.0;
  // 每拍时间*时间线缩放=拍距
  double beat_distance =
      beattime * editor_ref->cstatus.timeline_zoom *
      (editor_ref->cstatus.canvas_pasued ? 1.0
                                         : editor_ref->cstatus.speed_zoom);

  // 判定线位置
  auto judgeline_pos = editor_ref->cstatus.canvas_size.height() *
                       (1.0 - editor_ref->csettings.judgeline_position);

  // 距离此timing的拍数-1
  auto beat_count = int((editor_ref->cstatus.current_visual_time_stamp -
                         editor_ref->ebuffer.current_abs_timing->timestamp) /
                            beattime -
                        1);

  // TODO(xiang 2025-04-21):
  // 精确计算获取需要绘制的拍--保证不多不少(算入时间线缩放,变速缩放)
  // 当前处理的时间范围--大致
  editor_ref->ebuffer.current_time_area_start =
      editor_ref->ebuffer.current_abs_timing->timestamp + beat_count * beattime;
  editor_ref->ebuffer.current_time_area_end =
      editor_ref->ebuffer.current_abs_timing->timestamp + beat_count * beattime;

  // 拍距离判定线距离从下往上--反转
  // 拍起始位置
  double distance = std::fabs(editor_ref->ebuffer.current_time_area_end -
                              editor_ref->cstatus.current_visual_time_stamp);
  auto processing_pos =
      judgeline_pos + (distance * editor_ref->cstatus.timeline_zoom);

  while (processing_pos > -beattime) {
    editor_ref->ebuffer.current_time_area_end += beattime;
    processing_pos -= beat_distance;
  }

  // 清除拍缓存
  editor_ref->ebuffer.current_beats.clear();
  // 更新拍列表
  editor_ref->canvas_ref->working_map->query_beat_in_range(
      editor_ref->ebuffer.current_beats,
      int32_t(editor_ref->ebuffer.current_time_area_start),
      int32_t(editor_ref->ebuffer.current_time_area_end));
}
