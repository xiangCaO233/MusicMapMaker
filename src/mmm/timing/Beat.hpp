#ifndef MMM_BEAT_HPP
#define MMM_BEAT_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
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
using BeatHandle = int32_t;
using BeatInfo = std::unordered_map<uint32_t, Beat>;
using BeatTimeline = std::vector<uint32_t>;

// 定义一个结构体来存储查询结果
struct DivisorLineInfo {
    uint32_t beat_start;
    double divisor_time;     // 分拍线的时间
    uint32_t divisor_index;  // 第几个分拍线 (0 到 divisors-1)
    double distance;         // 查询时间与分拍线的距离
    bool is_valid() const {
        return distance != std::numeric_limits<double>::infinity();
    }
};

// 计算某个 Beat 的所有分拍线时间
inline std::vector<double> getDivisorTimesForBeat(const Beat& beat) {
    std::vector<double> times;
    if (beat.divisors == 0) return times;

    double divisor_interval = beat.beat_length / beat.divisors;
    for (uint32_t i = 0; i < beat.divisors; ++i) {
        times.push_back(beat.beat_start + i * divisor_interval);
    }
    return times;
}

inline DivisorLineInfo findNearestDivisorLine(int64_t query_time,
                                              const BeatTimeline& beat_timeline,
                                              const BeatInfo& beat_info) {
    if (beat_timeline.empty() || beat_info.empty()) {
        return {
            0, 0.0, 0,
            std::numeric_limits<double>::infinity()};  // 返回一个无效的默认值
    }

    double min_distance = std::numeric_limits<double>::infinity();
    DivisorLineInfo nearest_line = {0, 0.0, 0, min_distance};

    // 1. 找到包含 query_time 的 Beat
    auto it = std::upper_bound(beat_timeline.begin(), beat_timeline.end(),
                               query_time);

    // 确定要检查的 Beat 及其相邻 Beat 的索引
    std::vector<const Beat*> beats_to_check;

    // 添加当前 Beat
    if (it == beat_timeline.begin()) {
        // query_time 在第一个 Beat 之前或等于第一个 Beat 的开始时间
        // 此时 it 指向第一个元素（如果非空）
        // 仍然检查第一个 Beat
        if (!beat_timeline.empty()) {
            beats_to_check.push_back(&beat_info.at(*beat_timeline.begin()));
        }
    } else {
        // query_time 落在某个 Beat 内部或其边界
        it--;  // 指向包含 query_time 的 beat_start
        beats_to_check.push_back(&beat_info.at(*it));
    }

    // 添加前一个 Beat (如果存在)
    auto current_beat_it = std::find(beat_timeline.begin(), beat_timeline.end(),
                                     beats_to_check[0]->beat_start);
    if (current_beat_it != beat_timeline.begin()) {
        beats_to_check.push_back(&beat_info.at(*(--current_beat_it)));
    }

    // 添加后一个 Beat (如果存在)
    current_beat_it = std::find(beat_timeline.begin(), beat_timeline.end(),
                                beats_to_check[0]->beat_start);
    if (current_beat_it != beat_timeline.end() &&
        std::next(current_beat_it) != beat_timeline.end()) {
        beats_to_check.push_back(&beat_info.at(*std::next(current_beat_it)));
    }

    // 2. 遍历所有要检查的 Beat，计算它们的分拍线，并找到最近的一个
    for (const Beat* beat_ptr : beats_to_check) {
        if (!beat_ptr) continue;  // 安全检查

        const Beat& beat = *beat_ptr;
        std::vector<double> divisor_times = getDivisorTimesForBeat(beat);

        for (uint32_t i = 0; i < divisor_times.size(); ++i) {
            double dist =
                std::abs(static_cast<double>(query_time) - divisor_times[i]);
            if (dist < min_distance) {
                min_distance = dist;
                nearest_line = {beat.beat_start, divisor_times[i], i,
                                min_distance};
            }
        }
    }

    return nearest_line;
}

#endif  // !MMM_BEAT_HPP
