#ifndef MMM_TIMEPIXELCONVERTER_HPP
#define MMM_TIMEPIXELCONVERTER_HPP

#include <info/SharedCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>
#include <utility>

class TimePixelConverter {
   public:
    // 基准：在没有任何速度修饰时，1ms对应1px
    static constexpr double BASE_PIXELS_PER_MS = 1.0;

    // 基准BPM的参考拍长 (150 BPM = 400ms/beat)，用于计算BPM速度乘数
    static constexpr double REFERENCE_BEAT_DURATION_MS = 375.0;

    /**
     * @brief 构造函数。只保存必要的引用和参数，无复杂计算。
     * @param timings 谱面的TimingMap。
     * @param status 当前的画布状态。
     * @param prebpm 谱面的基准BPM。
     */
    TimePixelConverter(const TimingMap& timings, const BaseCanvasStatus& status,
                       double prebpm)
        : m_timings(timings), m_status(status) {
        if (prebpm > 0) {
            Timing initial_timing;
            initial_timing.is_base_timing = true;
            initial_timing.timestamp = 0;
            initial_timing.beat_length = 60000.0 / prebpm;
            initial_timing.bpm = prebpm;
            m_initial_timing = initial_timing;
        }
    }

    /**
     * @brief [核心] 将时间戳转换为相对于判定线的屏幕像素位置 (突变式)。
     */
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        // 1. 找出在当前判定线时间下，唯一生效的Timing点
        const Timing* active_timing =
            get_effective_timing_point(current_canvas_time);
        if (!active_timing) {
            // 如果连初始BPM都没有，无法计算，返回0偏移
            return 0.0f;
        }

        // 2. 计算在该Timing点控制下的瞬时速度 (pixels/ms)
        double pixels_per_ms = get_pixels_per_ms(*active_timing);

        // 3. 计算时间差
        double time_delta_ms = timestamp - current_canvas_time;

        // 4. 应用线性公式：距离 = 时间差 * 速度
        double pixel_offset = time_delta_ms * pixels_per_ms;

        // 5. 反转Y轴方向以实现“下落式”效果，并应用全局缩放
        return static_cast<float>(pixel_offset * m_status.timeline_zoom);
    }

    /**
     * @brief [核心] 将相对于判定线的屏幕像素位置转换为时间戳 (突变式)。
     */
    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(m_status.timeline_zoom) < 1e-9) return current_canvas_time;

        // 1. 找出在当前判定线时间下，唯一生效的Timing点
        const Timing* active_timing =
            get_effective_timing_point(current_canvas_time);
        if (!active_timing) {
            return current_canvas_time;
        }

        // 2. 计算在该Timing点控制下的瞬时速度 (pixels/ms)
        double pixels_per_ms = get_pixels_per_ms(*active_timing);
        if (std::abs(pixels_per_ms) < 1e-9) {
            return current_canvas_time;  // 避免除零
        }

        // 3. 反转Y轴和缩放，得到逻辑像素偏移
        double pixel_offset = pixel_y / m_status.timeline_zoom;

        // 4. 应用逆运算：时间差 = 距离 / 速度
        double time_delta_ms = pixel_offset / pixels_per_ms;

        return current_canvas_time + static_cast<int64_t>(time_delta_ms);
    }

   private:
    const TimingMap& m_timings;
    const BaseCanvasStatus& m_status;
    std::optional<Timing> m_initial_timing;

    /**
     * @brief 获取在指定时间戳下生效的Timing点，会回退到初始BPM。
     */
    const Timing* get_effective_timing_point(int64_t timestamp) const {
        const Timing* point = m_timings.get_active_timing_point(timestamp);
        if (point) return point;
        if (m_initial_timing.has_value()) return &m_initial_timing.value();
        return nullptr;
    }

    /**
     * @brief 查找一个Timing点所继承的基准红线，会回退到初始BPM。
     */
    const Timing* find_base_timing_for(int64_t timestamp) const {
        const auto& all_points = m_timings.get_all_timing_points();
        auto it = all_points.upper_bound(timestamp);
        while (it != all_points.begin()) {
            --it;
            if (it->second.is_base_timing) return &(it->second);
        }
        if (m_initial_timing.has_value()) return &m_initial_timing.value();
        return nullptr;
    }

    /**
     * @brief 根据一个Timing点，计算出其控制下的瞬时速度 (pixels/ms)。
     */
    double get_pixels_per_ms(const Timing& timing) const {
        // 1. 找到该Timing点对应的基准红线
        const Timing* base_timing =
            timing.is_base_timing ? &timing
                                  : find_base_timing_for(timing.timestamp);

        if (!base_timing || base_timing->beat_length <= 0) {
            // 如果没有有效的红线信息，则只受全局滚动速度影响
            return BASE_PIXELS_PER_MS * m_status.scroll_speed;
        }

        // 2. 计算BPM带来的速度乘数
        double base_beat_length = 60000.0 / base_timing->bpm;
        double reference_beat_length =
            (m_initial_timing.has_value() && m_initial_timing->beat_length > 0)
                ? (60000.0 / m_initial_timing->bpm)
                : REFERENCE_BEAT_DURATION_MS;
        double bpm_multiplier = reference_beat_length / base_beat_length;

        // 3. 计算绿线带来的速度乘数
        double velocity_multiplier = 1.0;
        if (!timing.is_base_timing && timing.beat_length < 0) {
            velocity_multiplier = -100.0 / timing.beat_length;
        }

        // 4. 组合所有乘数得到最终速度
        return BASE_PIXELS_PER_MS * m_status.scroll_speed * bpm_multiplier *
               velocity_multiplier;
    }
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
