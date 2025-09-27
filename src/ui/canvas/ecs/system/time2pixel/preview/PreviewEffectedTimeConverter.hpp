#ifndef MMM_PREVIEWEFFECTEDTIMECONVERTER_HPP
#define MMM_PREVIEWEFFECTEDTIMECONVERTER_HPP

#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>

/*
 * @class PreviewEffectedTimeConverter
 *        *****严重逻辑问题,我也不知道具体的逻辑了*****
 *        *****但是各种减来减去最后干活了,而且与渲染系统强接口耦合*****
 */
class PreviewEffectedTimeConverter : public TimePixelConverter {
   public:
    static constexpr double BASE_PIXELS_PER_MS = 1.0;

    PreviewEffectedTimeConverter(const TimingMap& timings,
                                 const MapCanvasInfo* info,
                                 const BaseCanvasStatus& status,
                                 const ScrollInfo& scrollInfo, double prebpm)
        : m_info(info), m_status(status), m_scrollInfo(scrollInfo) {
        const auto& editor_info = info->editorInfo;
        const auto& base_info = info->baseInfo;
        const float canvas_height = base_info.canvasSize.height();

        // 预览区相对主轨道的倍率
        auto maintrackpos_inpreview_area_ratio =
            editor_info.previewAreaInfo.areaRatio;
        // 主轨道在预览区中的高度
        auto maintrack_size_inpreview =
            canvas_height / maintrackpos_inpreview_area_ratio;
        // 主轨道中心在预览区中的倍率
        auto maintrackpos_inpreview_area =
            editor_info.previewAreaInfo.mainAreaPos;
        // 主轨道中心在预览区中的位置
        auto maintrack_center_inpreview =
            maintrackpos_inpreview_area * canvas_height;
        // 主轨道顶部在预览区中的位置
        auto maintrack_top_inpreview =
            maintrack_center_inpreview - maintrack_size_inpreview / 2.f;
        // 主轨道判定线在预览区中的位置
        judgeline_pos_in_previewarea =
            maintrack_top_inpreview +
            (1.f - editor_info.judgeline_pos) * maintrack_size_inpreview;
        buildLookupTable(timings, prebpm);
    }

    ~PreviewEffectedTimeConverter() override = default;

    float timeToPixel(int64_t timestamp, int64_t current_canvas_time,
                      const MapCanvasInfo* info) const override {
        double pixel_at_timestamp = getAbsolutePixelAt(timestamp);
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);
        double relative_pixel_offset =
            pixel_at_timestamp - pixel_at_current_time;
        auto untranslated_y = static_cast<float>(
            relative_pixel_offset * m_scrollInfo.timeline_zoom /
            info->editorInfo.previewAreaInfo.areaRatio);
        return info->baseInfo.canvasSize.height() - untranslated_y -
               (float(info->baseInfo.canvasSize.height()) -
                judgeline_pos_in_previewarea);
    }

    int64_t distanceToTime(float pixel_y,
                           int64_t current_canvas_time) const override {
        if (std::abs(m_scrollInfo.timeline_zoom *
                     m_info->editorInfo.previewAreaInfo.areaRatio) < 1e-9)
            return current_canvas_time;
        double pixel_at_current_time = getAbsolutePixelAt(current_canvas_time);
        double target_absolute_pixel =
            pixel_at_current_time +
            (pixel_y / m_scrollInfo.timeline_zoom *
             m_info->editorInfo.previewAreaInfo.areaRatio);
        return getTimeAtAbsolutePixel(target_absolute_pixel);
    }

   private:
    struct LookupNode {
        int64_t timestamp;
        double accumulated_pixels;
        double pixels_per_ms;
    };

    std::vector<LookupNode> m_lookup_table;

    float judgeline_pos_in_previewarea;
    const MapCanvasInfo* m_info;
    const BaseCanvasStatus& m_status;
    const ScrollInfo& m_scrollInfo;

    /**
     * @brief 构造函数的核心：构建累积像素距离的查找表。
     */
    void buildLookupTable(const TimingMap& timings, double prebpm) {
        m_lookup_table.clear();

        const auto& all_points = timings.get_all_timing_points();

        // 初始化状态变量
        double current_accumulated_pixels{.0};
        int64_t last_timestamp{0};

        // 初始速度由 prebpm 决定
        const auto preference_beat_length =
            (prebpm > 0) ? 60000.0 / prebpm : 0.0;
        auto last_pixels_per_ms =
            (preference_beat_length > 0)
                ? (BASE_PIXELS_PER_MS * m_scrollInfo.scroll_speed *
                   (preference_beat_length / preference_beat_length))
                : (BASE_PIXELS_PER_MS * m_scrollInfo.scroll_speed);

        // 维护当前生效的红线和绿线状态
        Timing current_base_timing;
        current_base_timing.beat_length = preference_beat_length;

        Timing current_inherited_timing;
        current_inherited_timing.beat_length = -100.0;  // 默认1.0x

        m_lookup_table.push_back({0, 0.0, last_pixels_per_ms});

        // 遍历所有时间点
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

            // 更新当前生效的红线和绿线状态
            for (const auto& timing : timing_list) {
                if (timing->is_base_timing) {
                    current_base_timing = *timing;
                } else {
                    current_inherited_timing = *timing;
                }
            }
            // 绿线的生效时间点如果早于红线，它会继承旧的红线BPM
            // 但如果同一时间点同时有红线和绿线，绿线应继承这个新的红线BPM
            if (current_inherited_timing.timestamp <
                current_base_timing.timestamp) {
                // 如果当前绿线比当前红线还早，说明它不跟这个红线走，重置为默认值
                current_inherited_timing.beat_length = -100.0;
            }

            // 根据最新的红线和绿线状态，计算新的速度
            double bpm_multiplier{1.0};
            static double last_bpm_multiplier{1.0};
            // 确保红线的 beat_length 是一个有效的正数
            if (current_base_timing.beat_length > 1e-2) {
                // 使用epsilon比较
                bpm_multiplier =
                    preference_beat_length / current_base_timing.beat_length;
            } else {
                // 如果红线BPM无效，可以选择继承上一个有效速度，或者使用默认值
                // 这里简单地保持 bpm_multiplier 为 1.0
                qWarning() << "在时间点" << timestamp
                           << "检测到无效的红线 beat_length:"
                           << current_base_timing.beat_length;
                bpm_multiplier = last_bpm_multiplier;
            }
            last_bpm_multiplier = bpm_multiplier;

            double velocity_multiplier{1.0};
            static double last_velocity_multiplier{1.0};
            // 确保绿线的 beat_length 是一个有效的、远离零的负数
            if (current_inherited_timing.beat_length < -1e-2) {
                velocity_multiplier =
                    -100.0 / current_inherited_timing.beat_length;
            } else if (current_inherited_timing.beat_length != -100.0) {
                qWarning() << "在时间点" << timestamp
                           << "检测到无效的绿线 beat_length:"
                           << current_inherited_timing.beat_length;
                // 强制为 1.0x 速度
                velocity_multiplier = last_velocity_multiplier;
            }
            last_velocity_multiplier = velocity_multiplier;

            double new_pixels_per_ms = BASE_PIXELS_PER_MS *
                                       m_scrollInfo.scroll_speed *
                                       bpm_multiplier * velocity_multiplier;

            // 确保速度不为零或接近零，也非无穷大或NaN
            if (std::abs(new_pixels_per_ms) < 1e-2) {
                qWarning()
                    << "在时间点" << timestamp
                    << "计算出的像素速度接近于零，强制设为最小值以防崩溃。";
                // 设置一个非常小的正值，而不是0
                new_pixels_per_ms = 1e-2;
            }
            if (!std::isfinite(new_pixels_per_ms)) {
                qWarning()
                    << "在时间点" << timestamp
                    << "计算出的像素速度为无穷大或NaN，强制重置为默认值。";
                // 若计算结果是 inf 或 NaN，回退到一个安全的速度
                new_pixels_per_ms = last_pixels_per_ms;
            }

            // 添加新节点到查找表
            m_lookup_table.push_back(
                {timestamp, current_accumulated_pixels, new_pixels_per_ms});

            last_timestamp = timestamp;
            last_pixels_per_ms = new_pixels_per_ms;
        }
    }

    double getAbsolutePixelAt(int64_t timestamp) const {
        if (m_lookup_table.empty())
            return -timestamp * BASE_PIXELS_PER_MS * m_scrollInfo.scroll_speed *
                   m_scrollInfo.timeline_zoom /
                   m_info->editorInfo.previewAreaInfo.areaRatio;

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
                absolute_pixel /
                (BASE_PIXELS_PER_MS * m_scrollInfo.scroll_speed));

        auto it = std::upper_bound(m_lookup_table.begin(), m_lookup_table.end(),
                                   absolute_pixel,
                                   [](double px, const LookupNode& node) {
                                       return px < node.accumulated_pixels;
                                   });

        if (it != m_lookup_table.begin()) --it;

        double pixels_into_segment = absolute_pixel - it->accumulated_pixels;
        if (std::abs(it->pixels_per_ms) < 1e-2) return it->timestamp;

        return it->timestamp +
               static_cast<int64_t>(pixels_into_segment / it->pixels_per_ms);
    }
};

#endif  // MMM_PREVIEWEFFECTEDTIMECONVERTER_HPP
