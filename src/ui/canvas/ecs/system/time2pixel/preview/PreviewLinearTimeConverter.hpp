#ifndef MMM_PREVIEWLINEARTIMECONVERTER_HPP
#define MMM_PREVIEWLINEARTIMECONVERTER_HPP

#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

/**
 * @class PreviewLinearTimeConverter
 * @brief 提供一个预览区的纯粹线性的时间到像素转换，不受任何游戏内变速效果影响。
 *        它只关心全局的时间缩放 (zoom)。
 *        *****严重逻辑问题,我也不知道具体的逻辑了*****
 *        *****但是各种减来减去最后干活了,而且与渲染系统强接口耦合*****
 */
class PreviewLinearTimeConverter : public TimePixelConverter {
   public:
    // 基准：在线性模式下，1ms对应多少像素 (不受scroll_speed影响)
    static constexpr double BASE_PIXELS_PER_MS_LINEAR = 1.0;

    /**
     * @brief 构造函数。
     * @param status 包含画布状态，主要使用 timeline_zoom。
     */
    explicit PreviewLinearTimeConverter(const MapCanvasInfo* info)
        : m_info(info) {
        const auto& editor_info = info->editorInfo;
        const auto& base_info = m_info->baseInfo;
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
    }
    ~PreviewLinearTimeConverter() override = default;

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

        const auto& editor_info = info->editorInfo;
        double untranslated_y =
            (pixel_at_timestamp_abs - pixel_at_current_time_abs) *
            editor_info.scrollInfo.timeline_zoom *
            editor_info.previewAreaInfo.areaRatio;

        const auto& base_info = m_info->baseInfo;
        const float canvas_height = base_info.canvasSize.height();

        return canvas_height - static_cast<float>(untranslated_y) -
               (canvas_height - judgeline_pos_in_previewarea);
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
        if (std::abs(editor_info.scrollInfo.timeline_zoom *
                     editor_info.previewAreaInfo.areaRatio) < 1e-9) {
            return current_canvas_time;
        }

        double untranslated_y_with_zoom = static_cast<double>(-pixel_y);

        double relative_pixel_offset =
            untranslated_y_with_zoom / (editor_info.scrollInfo.timeline_zoom *
                                        editor_info.previewAreaInfo.areaRatio);

        double time_delta_ms =
            relative_pixel_offset / BASE_PIXELS_PER_MS_LINEAR;

        return current_canvas_time - static_cast<int64_t>(time_delta_ms);
    }

   private:
    const MapCanvasInfo* m_info;
    float judgeline_pos_in_previewarea;
};
#endif  // MMM_PREVIEWLINEARTIMECONVERTER_HPP
