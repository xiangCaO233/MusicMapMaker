#ifndef MMM_BEAT_HPP
#define MMM_BEAT_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

class Timing;
struct Beat {
    // 拍开始时间
    uint32_t beat_start{0};
    double beat_length{100.};
    // 分拍数
    uint32_t divisors{2};
    // 是否手动
    bool is_manual{false};
    // 最近的生效timing
    const Timing* timing;
};
using BeatHandle = uint32_t;
using BeatInfo = std::unordered_map<uint32_t, Beat>;
using BeatTimeline = std::vector<uint32_t>;

#endif  // !MMM_BEAT_HPP
