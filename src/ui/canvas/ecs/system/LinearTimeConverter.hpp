#ifndef MMM_LINEARTIMECONVERTER_HPP
#define MMM_LINEARTIMECONVERTER_HPP

#include <info/MapCanvasInfo.hpp>

/**
 * @class LinearTimeConverter
 * @brief 提供一个纯粹线性的时间到像素转换，不受任何游戏内变速效果影响。
 *        它只关心全局的时间缩放 (zoom)。
 *        专门用于绘制时间轴、节拍线、Timing点等参考系元素。
 */
class LinearTimeConverter {
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
    float timeToPixel(int64_t timestamp, int64_t current_canvas_time) const {
        // --- 步骤 1: 计算纯粹的、线性的“相对像素偏移” ---

        // 1a. 计算纯粹的时间差
        double time_delta_ms = timestamp - current_canvas_time;

        // 1b. 将时间差乘以线性基准速度
        double relative_pixel_offset =
            time_delta_ms * BASE_PIXELS_PER_MS_LINEAR;

        // 1c. 应用全局的时间线缩放
        relative_pixel_offset *= m_info->baseInfo.timeline_zoom;

        return m_info->baseInfo.canvasSize.height() - relative_pixel_offset -
               (float(m_info->baseInfo.canvasSize.height()) -
                m_info->baseInfo.canvasSize.height() *
                    (1.f - m_info->baseInfo.judgeline_pos));
    }

    /**
     * @brief [核心] 将相对于判定线的屏幕像素位置转换为时间戳 (线性)。
     * @param pixel_y 屏幕Y轴上的相对像素偏移。
     * @param current_canvas_time 当前画布中心（判定线）的时间。
     * @return 计算出的时间戳。
     */
    int64_t pixelToTime(float pixel_y, int64_t current_canvas_time) const {
        if (std::abs(m_info->baseInfo.timeline_zoom) < 1e-9) {
            return current_canvas_time;
        }

        // --- 步骤 1: 执行 timeToPixel 的逆运算，从绝对坐标反算出相对偏移 ---
        const float judgeline_y = m_info->baseInfo.canvasSize.height() *
                                  m_info->baseInfo.judgeline_pos;
        double relative_pixel_offset = judgeline_y - pixel_y;

        // --- 步骤 2: 将相对偏移转换回时间差 ---

        // 2a. 逆转全局缩放
        relative_pixel_offset /= m_info->baseInfo.timeline_zoom;

        // 2b. 逆转线性速度换算
        double time_delta_ms =
            relative_pixel_offset / BASE_PIXELS_PER_MS_LINEAR;

        // 2c. 应用时间差
        return current_canvas_time + static_cast<int64_t>(time_delta_ms);
    }

   private:
    const MapCanvasInfo* m_info;
};
#endif  // MMM_LINEARTIMECONVERTER_HPP
