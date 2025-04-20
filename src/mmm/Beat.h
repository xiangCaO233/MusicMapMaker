#ifndef M_BEAT_H
#define M_BEAT_H

#include <compare>
#include <cstdint>

// "拍"结构体
struct Beat {
  // 此拍的bpm
  double bpm;
  // 拍起始时间
  double start_timestamp;
  // 拍结束时间
  double end_timestamp;
  // 自定义了小节线数
  bool divisors_customed{false};
  // 拍小节线数
  int32_t divisors{4};
  // 时间线缩放
  double timeline_zoom{1.0};

  // 操作符重载
  bool operator==(const Beat &other) const = default;
  bool operator<(const Beat &other) const {
    if (start_timestamp < other.start_timestamp) return true;
    if (start_timestamp == other.start_timestamp) {
      if (timeline_zoom == 1.0 && other.timeline_zoom != 1.0) {
        return true;
      }
      if (timeline_zoom != 1.0 && other.timeline_zoom == 1.0) {
        return false;
      }
    }

    return false;
  };
};

#endif  // M_BEAT_H
