#ifndef MMM_TIMEPIXELCONVERTER_HPP
#define MMM_TIMEPIXELCONVERTER_HPP

#include <info/SharedCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>
#include <utility>

class TimePixelConverter {
   public:
    // 基准：在没有任何速度修饰时，1ms对应1px
    static constexpr double BASE_PIXELS_PER_MS = 1.0;

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
     * @brief [O(logN)查询] 将时间戳转换为相对于判定线的屏幕像素位置
     */
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        // 使用查找表分别获取两个时间点的“绝对像素位置”（从t=0开始算）
        double pixel_at_timestamp = getAbsolutePixelAt(timestamp);
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);

        // 两者的差值就是它们的相对像素距离，最后应用全局缩放
        return static_cast<float>((pixel_at_current_time - pixel_at_timestamp) *
                                  m_status.timeline_zoom);
    }

    /**
     * @brief [O(logN)查询] 将相对于判定线的屏幕像素位置转换为时间戳。
     */
    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(m_status.timeline_zoom) < 1e-9) return current_canvas_time;

        // 1. 计算当前时间点在时间轴上的绝对像素位置
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);

        // 2. 根据相对偏移计算目标点的绝对像素位置
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

        // 1. 创建一个虚拟的 t=0 时间点作为计算的起点
        std::optional<Timing> initial_timing_opt;
        if (prebpm > 0) {
            Timing initial_timing;
            initial_timing.is_base_timing = true;
            initial_timing.timestamp = 0;
            initial_timing.beat_length = 60000.0 / prebpm;
            initial_timing.bpm = prebpm;
            initial_timing_opt = initial_timing;
        }

        double current_accumulated_pixels = 0.0;
        int64_t last_timestamp = 0;

        // 添加 t=0 节点到查找表
        double initial_pixels_per_ms =
            get_initial_pixels_per_ms(timings, initial_timing_opt);
        m_lookup_table.emplace_back(0, 0.0, initial_pixels_per_ms);

        // 2. 遍历所有真实的Timing点来构建表的其余部分
        for (const auto& [timestamp, timing] :
             timings.get_all_timing_points()) {
            if (timestamp > last_timestamp) {
                // 计算上一个区段的长度，并累加到总像素距离中
                double last_segment_speed = m_lookup_table.back().pixels_per_ms;
                current_accumulated_pixels +=
                    (timestamp - last_timestamp) * last_segment_speed;
            } else {
                // 处理同一时间戳有多个timing点的情况（虽然map不会，但逻辑上要健壮）
                // 此时累积距离不变，只更新速度
                current_accumulated_pixels =
                    m_lookup_table.back().accumulated_pixels;
                m_lookup_table.pop_back();  // 移除旧的，用新的覆盖
            }

            // 计算当前时间点开始的新速度
            double new_pixels_per_ms =
                get_pixels_per_ms(timing, timings, initial_timing_opt);

            // 添加新节点到查找表
            m_lookup_table.emplace_back(timestamp, current_accumulated_pixels,
                                        new_pixels_per_ms);
            last_timestamp = timestamp;
        }
    }

    /**
     * @brief [O(logK)] 根据时间戳获取从t=0开始的绝对像素位置 (K是Timing点数量)
     */
    double getAbsolutePixelAt(int64_t timestamp) const {
        if (m_lookup_table.empty())
            return timestamp * BASE_PIXELS_PER_MS * m_status.scroll_speed;

        // 在vector上进行二分查找，定位区段
        auto it =
            std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                             timestamp, [](int64_t ts, const LookupNode& node) {
                                 return ts < node.timestamp;
                             });

        if (it != m_lookup_table.begin()) {
            --it;  // it 现在指向包含 timestamp 的区段的起始节点
        }

        // 结果 = 区段起点的累积像素 + 在区段内的偏移像素
        return it->accumulated_pixels +
               (timestamp - it->timestamp) * it->pixels_per_ms;
    }

    /**
     * @brief [O(logK)] 根据从t=0开始的绝对像素位置反查时间戳
     */
    int64_t getTimeAtAbsolutePixel(double absolute_pixel) const {
        if (m_lookup_table.empty()) {
            return static_cast<int64_t>(
                absolute_pixel / (BASE_PIXELS_PER_MS * m_status.scroll_speed));
        }

        // 在vector上对累积像素进行二分查找
        auto it = std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                                   absolute_pixel,
                                   [](double px, const LookupNode& node) {
                                       return px < node.accumulated_pixels;
                                   });

        if (it != m_lookup_table.begin()) {
            --it;
        }

        double pixels_into_segment = absolute_pixel - it->accumulated_pixels;
        if (std::abs(it->pixels_per_ms) < 1e-9)
            return it->timestamp;  // 避免除零

        return it->timestamp +
               static_cast<int64_t>(pixels_into_segment / it->pixels_per_ms);
    }

    // --- 用于预计算的辅助函数 ---

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

        if (!base_timing || base_timing->beat_length <= 0) {
            return BASE_PIXELS_PER_MS * m_status.scroll_speed;
        }

        double base_beat_length = base_timing->beat_length;
        double reference_beat_length =
            (initial.has_value() && initial->beat_length > 0)
                ? initial->beat_length
                : 400.0;
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
        // 查找 t=0 时的真实timing点
        const Timing* real_timing_at_zero = timings.get_timing_point_at(0);
        if (real_timing_at_zero) {
            return get_pixels_per_ms(*real_timing_at_zero, timings, initial);
        }
        // 如果 t=0 没有点，则使用基准BPM
        if (initial.has_value()) {
            return get_pixels_per_ms(initial.value(), timings, initial);
        }
        // 最后的防线
        return BASE_PIXELS_PER_MS * m_status.scroll_speed;
    }
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
