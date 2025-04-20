#ifndef M_BEAT_H
#define M_BEAT_H

#include <cstdint>

// "拍"结构体
struct Beat {
  // 此拍的bpm
  double bpm;
  // 拍起始时间
  double start_timestamp;
  // 拍结束时间
  double end_timestamp;
  // 拍小节线数
  int32_t divisors{4};
  // 时间线缩放
  double timeline_zoom{1.0};

  bool operator==(const Beat &other) const = default;
};

#endif  // M_BEAT_H
