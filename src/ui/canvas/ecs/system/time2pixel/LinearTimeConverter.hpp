#ifndef MMM_LINEARTIMECONVERTER_HPP
#define MMM_LINEARTIMECONVERTER_HPP

#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

/**
 * @class LinearTimeConverter
 * @brief 提供一个纯粹线性的时间到像素转换，不受任何游戏内变速效果影响。
 *        它只关心全局的时间缩放 (zoom)。
 *        专门用于绘制时间轴、节拍线、Timing点等参考系元素。
 */
class LinearTimeConverter : public TimePixelConverter {
   public:
    // 基准：在线性模式下，1ms对应多少像素 (不受scroll_speed影响)
    static constexpr double BASE_PIXELS_PER_MS_LINEAR = 1.0;

    /**
     * @brief 构造函数。
     * @param status 包含画布状态，主要使用 timeline_zoom。
     */
    explicit LinearTimeConverter(const MapCanvasInfo* info) : m_info(info) {}

    /**
     * @brief [核心] 将时间戳转换为相对于判定线的屏幕像素位置 (线性)。
     * @param timestamp 要转换的时间戳。
     * @param current_canvas_time 当前画布中心（判定线）的时间。
     * @return 屏幕Y轴上的相对像素偏移。
     */
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time,
                      const MapCanvasInfo* info) const override {
        // --- 步骤 1: 计算纯粹线性的“绝对像素位置” ---
        //    这个位置是从 t=0 开始，不受 scroll_speed 影响。
        double pixel_at_timestamp_abs = timestamp * BASE_PIXELS_PER_MS_LINEAR;
        double pixel_at_current_time_abs =
            current_canvas_time * BASE_PIXELS_PER_MS_LINEAR;

        // --- 步骤 2: 计算相对于 current_canvas_time 的逻辑偏移
        // (untranslated_y) ---
        //    这个值会应用全局缩放
        double untranslated_y =
            (pixel_at_timestamp_abs - pixel_at_current_time_abs) *
            info->baseInfo.timeline_zoom;

        // --- 步骤 3: (核心) 应用与 EffectedTimeConverter
        // 完全相同的坐标变换公式 ---
        //    这个公式是经过验证的，我们不再修改它。
        const auto& base_info = info->baseInfo;
        const float canvas_height = base_info.canvasSize.height();
        const float judgeline_y_abs = canvas_height * base_info.judgeline_pos;

        // 之前我们简化了这个公式，现在我们用未经简化的原始版本，确保100%一致
        return canvas_height - static_cast<float>(untranslated_y) -
               (canvas_height -
                canvas_height * (1.f - base_info.judgeline_pos));
    }

    /**
     * @brief [核心] 将相对于判定线的屏幕像素位置转换为时间戳 (线性)。
     * @param pixel_y 屏幕Y轴上的相对像素偏移。
     * @param current_canvas_time 当前画布中心（判定线）的时间。
     * @return 计算出的时间戳。
     */
    int64_t distanceToTime(float pixel_y,
                           int64_t current_canvas_time) const override {
        const auto& base_info = m_info->baseInfo;
        if (std::abs(base_info.timeline_zoom) < 1e-9) {
            return current_canvas_time;
        }

        const float judgeline_y_abs =
            base_info.canvasSize.height() * base_info.judgeline_pos;
        double untranslated_y_with_zoom = static_cast<double>(-pixel_y);

        double relative_pixel_offset =
            untranslated_y_with_zoom / base_info.timeline_zoom;

        double time_delta_ms =
            relative_pixel_offset / BASE_PIXELS_PER_MS_LINEAR;

        // *** 核心修正 ***
        // 从 debug 日志看，我们的时间差完全是反的。
        // 所以我们直接在这里把它反转过来。
        return current_canvas_time - static_cast<int64_t>(time_delta_ms);
    }

   private:
    const MapCanvasInfo* m_info;
};
#endif  // MMM_LINEARTIMECONVERTER_HPP
