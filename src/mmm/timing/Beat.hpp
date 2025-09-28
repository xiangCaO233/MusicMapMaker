#ifndef MMM_BEAT_HPP
#define MMM_BEAT_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

class Timing;
struct Beat {
    // 拍号
    uint32_t beat_index;
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
    int64_t beat_start;
    double divisor_time;    // 分拍线的时间
    int64_t divisor_index;  // 第几个分拍线 (0 到 divisors-1)
    double distance{
        std::numeric_limits<double>::infinity()};  // 查询时间与分拍线的距离
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
    if (query_time < 0 || beat_timeline.empty() || beat_info.empty()) {
        return {
            -1, -1, -1,
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

// 用于指定查询方向的枚举
enum class SearchDirection {
    PREVIOUS,  // 向前查找（时间值更小的方向）
    AFTER      // 向后查找（时间值更大的方向）
};

/**
 * @brief 在指定时间位置的指定方向查找最近的分拍线(包括拍头线)
 *
 * @param query_time  查询的参考时间点
 * @param direction   查询方向 (PREVIOUS 或 AFTER)
 * @param beat_info   包含所有 Beat 信息的数据集合
 * @param tolerance_ms 一个小的容差值(毫秒), 用于判断两个时间点是否"足够接近"
 * @return DivisorLineInfo 找到的分拍线信息, 如果找不到则返回一个
 * is_valid()=false 的对象
 */
inline DivisorLineInfo findNearestDivisorLineInDirection(
    int64_t query_time, SearchDirection direction, const BeatInfo& beat_info,
    double tolerance_ms = 5.0) {  // 新增 tolerance_ms 参数

    if (query_time < 0 || beat_info.empty()) {
        return {-1, -1.0, -1, std::numeric_limits<double>::infinity()};
    }

    DivisorLineInfo result = {-1, -1.0, -1,
                              std::numeric_limits<double>::infinity()};
    const double query_time_d = static_cast<double>(query_time);

    if (direction == SearchDirection::AFTER) {
        double min_time_after = std::numeric_limits<double>::infinity();

        for (const auto& pair : beat_info) {
            const Beat& beat = pair.second;
            const auto divisor_times = getDivisorTimesForBeat(beat);

            for (uint32_t i = 0; i < divisor_times.size(); ++i) {
                const double line_time = divisor_times[i];
                // 核心修改点: 寻找第一个严格大于 (查询时间 + 容差) 的线
                if (line_time > query_time_d + tolerance_ms) {
                    if (line_time < min_time_after) {
                        min_time_after = line_time;
                        result = {static_cast<int64_t>(beat.beat_start),
                                  line_time, static_cast<int64_t>(i),
                                  std::abs(line_time - query_time_d)};
                    }
                }
            }
        }
    } else {  // SearchDirection::PREVIOUS
        DivisorLineInfo strictly_before_result = {
            -1, -1.0, -1, std::numeric_limits<double>::infinity()};
        DivisorLineInfo approx_equal_result = {
            -1, -1.0, -1, std::numeric_limits<double>::infinity()};
        double max_time_before = -1.0;

        for (const auto& pair : beat_info) {
            const Beat& beat = pair.second;
            const auto divisor_times = getDivisorTimesForBeat(beat);

            for (uint32_t i = 0; i < divisor_times.size(); ++i) {
                const double line_time = divisor_times[i];

                // 核心修改点 1: 寻找严格小于 (查询时间 - 容差) 的线
                if (line_time < query_time_d - tolerance_ms) {
                    if (line_time > max_time_before) {
                        max_time_before = line_time;
                        strictly_before_result = {
                            static_cast<int64_t>(beat.beat_start), line_time,
                            static_cast<int64_t>(i),
                            std::abs(line_time - query_time_d)};
                    }
                }
                // 核心修改点 2: 将 "等于" 的判断扩展为一个容差范围
                else if (std::abs(line_time - query_time_d) <= tolerance_ms) {
                    approx_equal_result = {
                        static_cast<int64_t>(beat.beat_start), line_time,
                        static_cast<int64_t>(i),
                        std::abs(line_time - query_time_d)};
                }
            }
        }

        // 优先返回严格小于的结果, 其次返回在容差范围内的结果
        if (strictly_before_result.is_valid()) {
            return strictly_before_result;
        }
        return approx_equal_result;
    }

    return result;
}

#endif  // !MMM_BEAT_HPP
