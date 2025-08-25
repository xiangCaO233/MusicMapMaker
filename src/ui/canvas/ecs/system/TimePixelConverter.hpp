#ifndef MMM_TIMEPIXELCONVERTER_HPP
#define MMM_TIMEPIXELCONVERTER_HPP

#include <info/SharedCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>

class TimePixelConverter {
   public:
    static constexpr double BASE_PIXELS_PER_MS = 1.0;

    TimePixelConverter(const TimingMap& timings, const BaseCanvasStatus& status,
                       double prebpm)
        : m_status(status) {
        buildLookupTable(timings, prebpm);
    }

    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        double pixel_at_timestamp = getAbsolutePixelAt(timestamp);
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);
        double relative_pixel_offset =
            pixel_at_timestamp - pixel_at_current_time;
        return static_cast<float>(relative_pixel_offset *
                                  m_status.timeline_zoom);
    }

    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(m_status.timeline_zoom) < 1e-9) return current_canvas_time;
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);
        double target_absolute_pixel =
            pixel_at_current_time + (pixel_y / m_status.timeline_zoom);
        return getTimeAtAbsolutePixel(target_absolute_pixel);
    }

   private:
    struct LookupNode {
        int64_t timestamp;
        double accumulated_pixels;
        double pixels_per_ms;
    };
    std::vector<LookupNode> m_lookup_table;

    const BaseCanvasStatus& m_status;

    /**
     * @brief 构造函数的核心：构建累积像素距离的查找表。
     */
    void buildLookupTable(const TimingMap& timings, double prebpm) {
        m_lookup_table.clear();

        const auto& all_points = timings.get_all_timing_points();

        // 1. 初始化状态变量
        double current_accumulated_pixels = 0.0;
        int64_t last_timestamp = 0;

        // 初始速度由 prebpm 决定
        const double preference_beat_length =
            (prebpm > 0) ? 60000.0 / prebpm : 0.0;
        double last_pixels_per_ms =
            (preference_beat_length > 0)
                ? (BASE_PIXELS_PER_MS * m_status.scroll_speed *
                   (preference_beat_length / preference_beat_length))
                : (BASE_PIXELS_PER_MS * m_status.scroll_speed);

        // 维护当前生效的红线和绿线状态
        Timing current_base_timing;
        current_base_timing.beat_length = preference_beat_length;

        Timing current_inherited_timing;
        current_inherited_timing.beat_length = -100.0;  // 默认1.0x

        m_lookup_table.push_back({0, 0.0, last_pixels_per_ms});

        // 2. 遍历所有时间点
        for (const auto& [timestamp, timing_list] : all_points) {
            if (timestamp > last_timestamp) {
                // 计算并累加上一个区段的像素距离
                current_accumulated_pixels +=
                    (timestamp - last_timestamp) * last_pixels_per_ms;
            } else if (timestamp == last_timestamp && !m_lookup_table.empty()) {
                // 如果是同一时间戳，回退一个节点，用合并后的新速度覆盖
                current_accumulated_pixels =
                    m_lookup_table.back().accumulated_pixels;
                m_lookup_table.pop_back();
            }

            // 3. 更新当前生效的红线和绿线状态
            for (const auto& timing : timing_list) {
                if (timing.is_base_timing) {
                    current_base_timing = timing;
                } else {
                    current_inherited_timing = timing;
                }
            }
            // 重要：绿线的生效时间点如果早于红线，它会继承旧的红线BPM
            // 但如果同一时间点同时有红线和绿线，绿线应继承这个新的红线BPM
            if (current_inherited_timing.timestamp <
                current_base_timing.timestamp) {
                // 如果当前绿线比当前红线还早，说明它不跟这个红线走，重置为默认值
                current_inherited_timing.beat_length = -100.0;
            }

            // 4. 根据最新的红线和绿线状态，计算新的速度
            double bpm_multiplier =
                (current_base_timing.beat_length > 0)
                    ? (preference_beat_length / current_base_timing.beat_length)
                    : 1.0;

            double velocity_multiplier =
                (current_inherited_timing.beat_length < 0)
                    ? (-100.0 / current_inherited_timing.beat_length)
                    : 1.0;

            double new_pixels_per_ms = BASE_PIXELS_PER_MS *
                                       m_status.scroll_speed * bpm_multiplier *
                                       velocity_multiplier;

            // 5. 添加新节点到查找表
            m_lookup_table.push_back(
                {timestamp, current_accumulated_pixels, new_pixels_per_ms});

            last_timestamp = timestamp;
            last_pixels_per_ms = new_pixels_per_ms;
        }
    }

    double getAbsolutePixelAt(int64_t timestamp) const {
        if (m_lookup_table.empty())
            return -timestamp * BASE_PIXELS_PER_MS * m_status.scroll_speed *
                   m_status.timeline_zoom;

        auto it =
            std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                             timestamp, [](int64_t ts, const LookupNode& node) {
                                 return ts < node.timestamp;
                             });

        if (it != m_lookup_table.begin()) --it;

        return it->accumulated_pixels +
               (timestamp - it->timestamp) * it->pixels_per_ms;
    }

    int64_t getTimeAtAbsolutePixel(double absolute_pixel) const {
        if (m_lookup_table.empty())
            return static_cast<int64_t>(
                absolute_pixel / (BASE_PIXELS_PER_MS * m_status.scroll_speed));

        auto it = std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                                   absolute_pixel,
                                   [](double px, const LookupNode& node) {
                                       return px < node.accumulated_pixels;
                                   });

        if (it != m_lookup_table.begin()) --it;

        double pixels_into_segment = absolute_pixel - it->accumulated_pixels;
        if (std::abs(it->pixels_per_ms) < 1e-9) return it->timestamp;

        return it->timestamp +
               static_cast<int64_t>(pixels_into_segment / it->pixels_per_ms);
    }
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
