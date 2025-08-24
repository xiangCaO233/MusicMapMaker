#ifndef MMM_MAPCANVASCLOCK_HPP
#define MMM_MAPCANVASCLOCK_HPP

#include <info/SharedCanvasInfo.hpp>

class MapCanvasClock {
   public:
    MapCanvasClock() = default;

    /**
     * @brief 每帧调用一次，用于更新画布的呈现时间。
     * @param info 包含所有实时状态的共享结构。
     * @param frame_delta_time_ms 上一帧到这一帧的实际间隔时间 (ms)。
     */
    void update(RealTimeInfo& info, double smoothed_delta_ms) {
        if (!info.is_playing) {
            // --- 暂停状态逻辑 ---
            if (m_was_playing) {
                // 这是从“播放”切换到“暂停”的第一帧
                // 在这里执行一次强制的、最终的同步
                const double final_target_time = info.raw_audio_time_ms.load() +
                                                 info.global_offset_ms.load();
                info.current_canvas_time = final_target_time;

                // 重置状态
                reset();
            }
            // 更新状态标记
            m_was_playing = false;
            return;
        }

        // --- 播放状态逻辑 ---
        m_was_playing = true;  // 标记当前正在播放

        // 2. 先让时钟按平滑、可预测的速度正常前进
        info.current_canvas_time += smoothed_delta_ms;

        // 3. 计算与“真实”音频时间的目标差距
        const double target_time =
            info.raw_audio_time_ms.load() + info.global_offset_ms.load();

        // 如果音频时间重置了（比如循环），我们的时钟也应该重置
        if (target_time < m_last_target_time) {
            info.current_canvas_time = target_time;
            reset();
        }
        m_last_target_time = target_time;

        const double error = target_time - info.current_canvas_time;

        // 4. 使用一个极其简单的 P-控制器 (比例) 来计算一个微小的修正量
        //    因为输入delta已经平滑，我们不再需要复杂的I和D项来防抖
        const double correction = error * Kp;

        // 5. 将这个微小的修正量应用到画布时间上
        //    我们不再修改速度，而是直接微调时间本身，这更稳定
        info.current_canvas_time += correction;
    }

    // 公共的重置函数，用于暂停或seek等操作
    void reset() { m_last_target_time = 0.0; }

   private:
    // --- 更柔和的PID参数 ---
    // P: 比例项决定了基础的响应速度
    const double Kp = 0.25;
    // 新增状态，用于检测播放/暂停的切换
    bool m_was_playing = false;

    double m_last_target_time = 0.0;
};

#endif  // MMM_MAPCANVASCLOCK_HPP
