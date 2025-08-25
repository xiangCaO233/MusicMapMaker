#ifndef MMM_MAPCANVASCLOCK_HPP
#define MMM_MAPCANVASCLOCK_HPP

#include <info/SharedCanvasInfo.hpp>

class MapCanvasClock {
   public:
    MapCanvasClock() { reset(); };

    /**
     * @brief 每帧调用一次，用于更新画布的呈现时间。
     * @param info 包含所有实时状态的共享结构。
     * @param frame_delta_time_ms 上一帧到这一帧的实际间隔时间 (ms)。
     */
    void update(RealTimeInfo& info, double smoothed_delta_ms) {
        if (!info.is_playing) {
            if (m_was_playing) {
                // 暂停时，强制将逻辑时间与原始音频时间同步
                info.logic_canvas_time = info.raw_audio_time_ms.load();
            }
            reset();
            return;
        }

        double current_playback_rate = info.audio_playback_rate.load();

        if (!m_was_playing || m_last_playback_rate != current_playback_rate) {
            // 进入播放或变速时，重置逻辑时间
            m_was_playing = true;
            m_last_playback_rate = current_playback_rate;

            info.logic_canvas_time = info.raw_audio_time_ms.load();
            m_last_known_audio_time = info.raw_audio_time_ms.load();
            m_canvas_time_at_last_sync =
                info.logic_canvas_time;  // 使用逻辑时间
            m_rate_corrector = 0.0;
        }

        // --- 校准事件：只在原始音频时间上操作 ---
        double current_audio_time = info.raw_audio_time_ms.load();
        bool has_new_audio_update =
            (current_audio_time != m_last_known_audio_time);

        if (has_new_audio_update) {
            // 目标时间现在就是原始音频时间，没有任何偏移！
            double target_time = current_audio_time;

            // 相位误差：我们的逻辑时间与原始音频时间的差距
            double phase_error = target_time - info.logic_canvas_time;

            // ... (频率误差的计算完全不变，因为它使用的都是相对变化量) ...
            double audio_elapsed = current_audio_time - m_last_known_audio_time;
            double canvas_elapsed =
                info.logic_canvas_time - m_canvas_time_at_last_sync;
            if (audio_elapsed > 0 && canvas_elapsed > 0) {
                // ... (freq_error, m_rate_corrector 的更新不变) ...
            }

            // 应用位置修正到逻辑时间
            double position_correction = phase_error * Kp;
            info.logic_canvas_time += position_correction;

            // 更新同步点
            m_last_known_audio_time = current_audio_time;
            m_canvas_time_at_last_sync = info.logic_canvas_time;
        }

        // --- 预测性前进：只在逻辑时间上操作 ---
        m_clock_rate = current_playback_rate + m_rate_corrector;
        info.logic_canvas_time += smoothed_delta_ms * m_clock_rate;
    }

    // 公共的重置函数，用于暂停或seek等操作
    void reset() {
        m_was_playing = false;
        m_clock_rate = 1.0;
        m_rate_corrector = 0.0;
        m_last_known_audio_time = 0.0;
        m_canvas_time_at_last_sync = 0.0;
    }

   private:
    // P: 比例增益，用于修正相位（时间）误差
    const double Kp = 0.05;
    // I: 积分增益，用于修正频率（速度）误差
    const double Ki = 0.001;

    // --- 状态变量 ---
    bool m_was_playing = false;

    // 我们自己内部维护的、平滑的速度（单位：毫秒/毫秒，正常应为1.0）
    double m_clock_rate = 0.5;

    // 用于修正速度的积分项
    double m_rate_corrector = 0.0;

    // 上一次接收到的、有效的音频时间
    double m_last_known_audio_time = 0.0;

    // 上一次接收到音频时间时，我们自己的画布时间是多少
    double m_canvas_time_at_last_sync = 0.0;

    // 新增成员，用于检测播放速率的变化
    double m_last_playback_rate = 1.0;
};

#endif  // MMM_MAPCANVASCLOCK_HPP
