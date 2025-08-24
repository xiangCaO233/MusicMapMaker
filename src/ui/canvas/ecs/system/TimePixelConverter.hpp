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
    static constexpr double REFERENCE_BEAT_DURATION_MS = 400.0;

    /**
     * @brief 构造函数。所有昂贵的计算都在这里一次性完成。
     * @param timings 谱面的TimingMap。
     * @param status 当前的画布状态。
     * @param prebpm 谱面的基准BPM，用于处理第一个Timing点之前的区域。
     */
    TimePixelConverter(const TimingMap& timings, const BaseCanvasStatus& status,
                       double prebpm)
        : m_status(status) {
        buildLookupTable(timings, prebpm);
    }

    /**
     * @brief [O(logN)查询] 将时间戳转换为相对于判定线的屏幕像素位置。
     */
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        // 使用查找表分别获取两个时间点的“绝对像素位置”（从t=0开始算）
        double pixel_at_timestamp = getAbsolutePixelAt(timestamp);
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);

        // 两者的差值就是它们的相对像素距离
        double relative_pixel_offset =
            pixel_at_timestamp - pixel_at_current_time;

        return static_cast<float>(relative_pixel_offset *
                                  m_status.timeline_zoom);
    }

    /**
     * @brief [O(logN)查询] 将相对于判定线的屏幕像素位置转换为时间戳。
     */
    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(m_status.timeline_zoom) < 1e-9) return current_canvas_time;

        // 1. 计算当前时间点在时间轴上的绝对像素位置
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);

        // 2. 根据相对偏移计算目标点的绝对像素位置 (注意反转Y轴)
        double target_absolute_pixel =
            pixel_at_current_time + (pixel_y / m_status.timeline_zoom);

        // 3. 在查找表中通过二分查找反算出时间戳
        return getTimeAtAbsolutePixel(target_absolute_pixel);
    }

   private:
    // --- 内部数据结构：快速查找表 ---
    struct LookupNode {
        int64_t timestamp;
        double accumulated_pixels;  // 从 t=0 到此时间点的总像素距离 (无zoom)
        double pixels_per_ms;       // 从此时间点开始的区段的速度
    };
    std::vector<LookupNode> m_lookup_table;

    // 实时状态仍然需要引用，因为它每帧都可能变化
    const BaseCanvasStatus& m_status;

    /**
     * @brief 构造函数的核心：构建累积像素距离的查找表。
     */
    void buildLookupTable(const TimingMap& timings, double prebpm) {
        m_lookup_table.clear();

        std::optional<Timing> initial_timing_opt;
        if (prebpm > 0) {
            Timing t;
            t.is_base_timing = true;
            t.timestamp = 0;
            t.beat_length = 60000.0 / prebpm;
            t.bpm = prebpm;
            initial_timing_opt = t;
        }

        double current_accumulated_pixels = 0.0;
        int64_t last_timestamp = 0;

        // 添加 t=0 节点到查找表
        double initial_pixels_per_ms =
            get_initial_pixels_per_ms(timings, initial_timing_opt);
        m_lookup_table.push_back({0, 0.0, initial_pixels_per_ms});

        // 遍历所有真实的Timing点来构建表的其余部分
        for (const auto& [timestamp, timing] :
             timings.get_all_timing_points()) {
            if (timestamp <= last_timestamp) {  // 处理同一时间戳上的点
                current_accumulated_pixels =
                    m_lookup_table.back().accumulated_pixels;
                m_lookup_table.pop_back();
            } else {  // 处理新的时间戳
                double last_segment_speed = m_lookup_table.back().pixels_per_ms;
                current_accumulated_pixels +=
                    (timestamp - last_timestamp) * last_segment_speed;
            }

            double new_pixels_per_ms =
                get_pixels_per_ms(timing, timings, initial_timing_opt);
            m_lookup_table.push_back(
                {timestamp, current_accumulated_pixels, new_pixels_per_ms});
            last_timestamp = timestamp;
        }
    }

    /**
     * @brief [O(logK)] 根据时间戳获取从t=0开始的绝对像素位置
     */
    double getAbsolutePixelAt(int64_t timestamp) const {
        if (m_lookup_table.empty())
            return timestamp * BASE_PIXELS_PER_MS * m_status.scroll_speed;

        auto it =
            std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                             timestamp, [](int64_t ts, const LookupNode& node) {
                                 return ts < node.timestamp;
                             });

        if (it != m_lookup_table.begin()) --it;

        return it->accumulated_pixels +
               (timestamp - it->timestamp) * it->pixels_per_ms;
    }

    /**
     * @brief [O(logK)] 根据从t=0开始的绝对像素位置反查时间戳
     */
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

    // --- 用于预计算的辅助函数 (与突变式模型中的逻辑完全相同) ---

    const Timing* find_base_timing_for(
        int64_t timestamp, const TimingMap& timings,
        const std::optional<Timing>& initial) const {
        const auto& all_points = timings.get_all_timing_points();
        auto it = all_points.upper_bound(timestamp);
        while (it != all_points.begin()) {
            --it;
            if (it->second.is_base_timing) return &(it->second);
        }
        if (initial.has_value()) return &initial.value();
        return nullptr;
    }

    double get_pixels_per_ms(const Timing& timing, const TimingMap& timings,
                             const std::optional<Timing>& initial) const {
        const Timing* base_timing =
            timing.is_base_timing
                ? &timing
                : find_base_timing_for(timing.timestamp, timings, initial);
        if (!base_timing || base_timing->beat_length <= 0)
            return BASE_PIXELS_PER_MS * m_status.scroll_speed;

        double base_beat_length = base_timing->beat_length;
        double reference_beat_length =
            (initial.has_value() && initial->beat_length > 0)
                ? initial->beat_length
                : REFERENCE_BEAT_DURATION_MS;
        double bpm_multiplier = reference_beat_length / base_beat_length;

        double velocity_multiplier = 1.0;
        if (!timing.is_base_timing && timing.beat_length < 0) {
            velocity_multiplier = -100.0 / timing.beat_length;
        }
        return BASE_PIXELS_PER_MS * m_status.scroll_speed * bpm_multiplier *
               velocity_multiplier;
    }

    double get_initial_pixels_per_ms(
        const TimingMap& timings, const std::optional<Timing>& initial) const {
        const Timing* real_timing_at_zero = timings.get_timing_point_at(0);
        if (real_timing_at_zero)
            return get_pixels_per_ms(*real_timing_at_zero, timings, initial);
        if (initial.has_value())
            return get_pixels_per_ms(initial.value(), timings, initial);
        return BASE_PIXELS_PER_MS * m_status.scroll_speed;
    }
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
