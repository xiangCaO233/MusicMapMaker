#ifndef MMM_TIMEPIXELCONVERTER_HPP
#define MMM_TIMEPIXELCONVERTER_HPP

#include <info/SharedCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>
class TimePixelConverter {
   public:
    static constexpr double BASE_PIXELS_PER_MS = 1.0;

    TimePixelConverter(const TimingMap& timings, const BaseCanvasStatus& status,
                       double prebpm)
        : m_timings(timings), m_status(status) {
        if (prebpm > 0) {
            Timing initial_timing;
            initial_timing.is_base_timing = true;
            initial_timing.timestamp = 0;  // 逻辑上的时间原点
            initial_timing.beat_length = 60000.0 / prebpm;
            initial_timing.bpm = prebpm;
            m_initial_timing = initial_timing;
        }
    }

    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        // ... 这部分的主体逻辑可以保持和你写的基本一致 ...
        // 关键是它内部调用的 get_pixels_per_ms 现在是健壮的
        // 为确保完整性，我们使用健壮的重写版本：
        if (timestamp == current_canvas_time) return 0.0f;

        bool is_forward = timestamp > current_canvas_time;
        int64_t start_time = is_forward ? current_canvas_time : timestamp;
        int64_t end_time = is_forward ? timestamp : current_canvas_time;

        double total_pixel_distance = 0.0;
        int64_t current_time = start_time;

        const auto& all_points = m_timings.get_all_timing_points();
        auto it = all_points.upper_bound(start_time);
        if (it != all_points.begin()) --it;

        while (current_time < end_time) {
            const Timing* active_timing =
                get_effective_timing_point(current_time);
            if (!active_timing) break;  // 理论上不会发生，除非初始BPM无效

            double pixels_per_ms = get_pixels_per_ms(*active_timing);

            auto next_it = all_points.upper_bound(current_time);
            int64_t segment_end_time =
                (next_it != all_points.end()) ? next_it->first : end_time;

            int64_t actual_end = std::min(segment_end_time, end_time);

            total_pixel_distance += (actual_end - current_time) * pixels_per_ms;
            current_time = actual_end;

            if (current_time >= end_time) break;
        }

        total_pixel_distance *= m_status.timeline_zoom;
        return is_forward ? static_cast<float>(total_pixel_distance)
                          : -static_cast<float>(total_pixel_distance);
    }

    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(pixel_y) < 1e-6) return current_canvas_time;
        if (std::abs(m_status.timeline_zoom) < 1e-6) return current_canvas_time;

        double target_pixel_distance = pixel_y / m_status.timeline_zoom;
        bool is_forward = target_pixel_distance > 0;
        target_pixel_distance = std::abs(target_pixel_distance);

        int64_t current_time = current_canvas_time;
        const auto& all_points = m_timings.get_all_timing_points();

        if (is_forward) {
            auto it = all_points.upper_bound(current_time);
            while (target_pixel_distance > 1e-6) {
                const Timing* active_timing =
                    get_effective_timing_point(current_time);
                if (!active_timing) return current_time;

                double pixels_per_ms = get_pixels_per_ms(*active_timing);
                if (std::abs(pixels_per_ms) < 1e-6) return current_time;

                int64_t next_timing_ts =
                    (it != all_points.end()) ? it->first : -1;
                double pixels_to_next_point =
                    (next_timing_ts != -1)
                        ? (next_timing_ts - current_time) * pixels_per_ms
                        : 1e18;

                if (target_pixel_distance <= pixels_to_next_point) {
                    return current_time +
                           static_cast<int64_t>(target_pixel_distance /
                                                pixels_per_ms);
                }
                target_pixel_distance -= pixels_to_next_point;
                current_time = next_timing_ts;
                if (it != all_points.end()) ++it;
            }
            return current_time;
        } else {
            while (target_pixel_distance > 1e-6) {
                const Timing* active_timing =
                    get_effective_timing_point(current_time);
                if (!active_timing) return current_time;

                double pixels_per_ms = get_pixels_per_ms(*active_timing);
                if (std::abs(pixels_per_ms) < 1e-6) return current_time;

                auto it = all_points.upper_bound(current_time);
                int64_t prev_timing_ts = -1;
                if (it != all_points.begin()) {
                    --it;  // it 是 <= current_time 的最后一个点
                    prev_timing_ts = it->first;
                }

                double pixels_to_prev_point =
                    (prev_timing_ts != -1)
                        ? (current_time - prev_timing_ts) * pixels_per_ms
                        : 1e18;

                if (target_pixel_distance <= pixels_to_prev_point) {
                    return current_time -
                           static_cast<int64_t>(target_pixel_distance /
                                                pixels_per_ms);
                }
                target_pixel_distance -= pixels_to_prev_point;
                current_time = prev_timing_ts;
            }
            return current_time;
        }
    }

   private:
    const TimingMap& m_timings;
    const BaseCanvasStatus& m_status;
    std::optional<Timing> m_initial_timing;

    /**
     * @brief [核心辅助函数]
     * 获取在指定时间戳下生效的Timing点，永远不会失败（除非没给初始BPM）。
     */
    const Timing* get_effective_timing_point(int64_t timestamp) const {
        const Timing* point = m_timings.get_active_timing_point(timestamp);
        if (point) return point;
        if (m_initial_timing.has_value()) return &m_initial_timing.value();
        return nullptr;
    }

    /**
     * @brief [核心辅助函数] 查找基准红线，会回退到初始Timing点。
     */
    const Timing* find_base_timing_for(int64_t timestamp) const {
        const auto& all_points = m_timings.get_all_timing_points();
        auto it = all_points.upper_bound(timestamp);
        while (it != all_points.begin()) {
            --it;
            if (it->second.is_base_timing) {
                return &(it->second);
            }
        }
        if (m_initial_timing.has_value()) return &m_initial_timing.value();
        return nullptr;
    }

    double get_pixels_per_ms(const Timing& timing) const {
        const Timing* base_timing =
            timing.is_base_timing ? &timing
                                  : find_base_timing_for(timing.timestamp);

        // 这里的 `base_timing` 现在几乎不可能为 null
        if (!base_timing || base_timing->beat_length <= 0) {
            return BASE_PIXELS_PER_MS * m_status.scroll_speed;
        }

        double base_beat_length = base_timing->beat_length;

        // 使用你写的公式，它在逻辑上是正确的
        // REFERENCE_BPM_BEAT_LENGTH / current_beat_length
        double reference_beat_length =
            (m_initial_timing.has_value() && m_initial_timing->beat_length > 0)
                ? m_initial_timing->beat_length
                : 400.0;  // 默认150bpm
        double bpm_multiplier = reference_beat_length / base_beat_length;

        double velocity_multiplier = 1.0;
        if (!timing.is_base_timing && timing.beat_length < 0) {
            velocity_multiplier = -100.0 / timing.beat_length;
        }
        return BASE_PIXELS_PER_MS * m_status.scroll_speed * bpm_multiplier *
               velocity_multiplier;
    }
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
