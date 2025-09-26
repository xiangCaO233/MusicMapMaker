#ifndef MMM_LINEARTIMECONVERTER_HPP
#define MMM_LINEARTIMECONVERTER_HPP

#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

/**
 * @class LinearTimeConverter
 * @brief 提供一个纯粹线性的时间到像素转换，不受任何游戏内变速效果影响。
 *        它只关心全局的时间缩放 (zoom)。
 *        *****严重逻辑问题,我也不知道具体的逻辑了*****
 *        *****但是各种减来减去最后干活了,而且与渲染系统强接口耦合*****
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
    ~LinearTimeConverter() override = default;

    /**
     * @brief [核心] 将时间戳转换为相对于判定线的屏幕像素位置 (线性)。
     * @param timestamp 要转换的时间戳。
     * @param current_canvas_time 当前画布中心（判定线）的时间。
     * @return 屏幕Y轴上的相对像素偏移。
     */
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time,
                      const MapCanvasInfo* info) const override {
        double pixel_at_timestamp_abs = timestamp * BASE_PIXELS_PER_MS_LINEAR;
        double pixel_at_current_time_abs =
            current_canvas_time * BASE_PIXELS_PER_MS_LINEAR;

        double untranslated_y =
            (pixel_at_timestamp_abs - pixel_at_current_time_abs) *
            info->editorInfo.scrollInfo.timeline_zoom;

        const auto& base_info = m_info->baseInfo;
        const auto& editor_info = info->editorInfo;
        const float canvas_height = base_info.canvasSize.height();
        const float judgeline_y_abs = canvas_height * editor_info.judgeline_pos;

        return canvas_height - static_cast<float>(untranslated_y) -
               (canvas_height -
                canvas_height * (1.f - editor_info.judgeline_pos));
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
        const auto& editor_info = m_info->editorInfo;
        if (std::abs(editor_info.scrollInfo.timeline_zoom) < 1e-9) {
            return current_canvas_time;
        }

        const float judgeline_y_abs =
            base_info.canvasSize.height() * editor_info.judgeline_pos;
        double untranslated_y_with_zoom = static_cast<double>(-pixel_y);

        double relative_pixel_offset =
            untranslated_y_with_zoom / editor_info.scrollInfo.timeline_zoom;

        double time_delta_ms =
            relative_pixel_offset / BASE_PIXELS_PER_MS_LINEAR;

        return current_canvas_time - static_cast<int64_t>(time_delta_ms);
    }

   private:
    const MapCanvasInfo* m_info;
};
#endif  // MMM_LINEARTIMECONVERTER_HPP
