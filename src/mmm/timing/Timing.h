#ifndef M_TIMING_H
#define M_TIMING_H

// 时间点
#include <cstdint>
class Timing {
 public:
  // 构造Timing
  Timing();
  // 析构Timing
  virtual ~Timing();

  // 该时间点的时间戳
  uint32_t timestamp;
  // 该时间点的bpm
  double bpm;
};

#endif  // M_TIMING_H
