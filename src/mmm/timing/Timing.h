#ifndef M_TIMING_H
#define M_TIMING_H

#include <compare>

// 时间点
#include <cstdint>

enum class TimingType {
  OSUTIMING,
  RMTIMING,
  MALODYTIMING,
  UNKNOWN,
};

class Timing {
 public:
  // 构造Timing
  Timing();
  // 析构Timing
  virtual ~Timing();

  // 是否为基准timing
  bool is_base_timing{true};

  // timing类型
  TimingType type;

  // 该时间点的时间戳
  // in-osu
  // *时间（整型）：
  // *时间轴区间的开始时间，以谱面音频开始为原点，单位是毫秒。
  // 这个时间轴区间的结束时间即为下一个时间点的开始时间（如果这是最后一个时间点，则无结束时间）。
  // in-imd
  //
  int32_t timestamp{-1};

  // 该时间点的bpm
  // in-osu
  // 拍长（精准小数）： 这个参数有两种含义：
  // 对于非继承时间点（红线），这个参数即一拍的长度，单位是毫秒。
  // 对于继承时间点（绿线），这个参数为负值，去掉符号后被 100
  // 整除，即为这根绿线控制的滑条速度。例如，-50
  // 代表这一时间段的滑条速度均为基础滑条速度的 2 倍。
  // in-imd
  //
  double bpm{-1.0};

  // 非继承的最近的基准bpm
  double basebpm{-1.0};

  bool operator==(const Timing& other) const = default;

  auto operator<=>(const Timing& other) const {
    return timestamp <=> other.timestamp;
  }
};

#endif  // M_TIMING_H
