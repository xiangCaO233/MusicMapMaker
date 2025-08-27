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
    void updateInt(RealTimeInfo& info, double smoothed_delta_ms) {
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
            double target_time = current_audio_time;
            double phase_error = target_time - info.logic_canvas_time;

            // 计算自上次同步以来，音频时间和画布时间各自前进了多少
            double audio_elapsed = current_audio_time - m_last_known_audio_time;
            double canvas_elapsed =
                info.logic_canvas_time - m_canvas_time_at_last_sync;

            // 只有在时间是前进的情况下才进行计算，以避免seek或loop导致的错误
            if (audio_elapsed > 0 && canvas_elapsed > 0) {
                // 期望的画布前进时间 = 音频前进时间 * 当前播放速率
                double expected_canvas_elapsed =
                    audio_elapsed * current_playback_rate;

                // 频率误差 = (实际前进量 / 期望前进量) - 1.0
                // 如果 > 0，说明我们的时钟跑快了
                // 如果 < 0，说明我们的时钟跑慢了
                double freq_error =
                    (canvas_elapsed / expected_canvas_elapsed) - 1.0;

                // 【I项核心】用积分项累积并修正这个频率(速度)误差
                // 用 -= 是因为如果时钟跑快了(freq_error >
                // 0)，需要减小修正量
                m_rate_corrector -= freq_error * Ki;

                // (可选但推荐) 对积分项进行范围限制，防止失控 (Windup Guard)
                // 限制修正量在 +/- 10% 的速率范围内
                const double max_rate_correction = 0.1;
                m_rate_corrector =
                    std::clamp(m_rate_corrector, -max_rate_correction,
                               max_rate_correction);
            }

            // 【P项】应用瞬时位置修正到逻辑时间
            double position_correction = phase_error * Kp;
            info.logic_canvas_time += position_correction;

            // 更新同步点信息
            m_last_known_audio_time = current_audio_time;
            m_canvas_time_at_last_sync = info.logic_canvas_time;
        }

        // --- 预测性前进：只在逻辑时间上操作 ---
        m_clock_rate = current_playback_rate + m_rate_corrector;
        info.logic_canvas_time += smoothed_delta_ms * m_clock_rate;
    }
    void updateWBox(RealTimeInfo& info, double smoothed_delta_ms) {
        if (!info.is_playing) {
            if (m_was_playing) {
                // 暂停时，强制同步一次。此时允许“跳变”。
                info.logic_canvas_time = info.raw_audio_time_ms.load();
            }
            reset();
            return;
        }

        // ---------------------------------------------------------------------
        // 核心逻辑: 不再区分是否有音频更新，每一帧都执行相同的平滑调整
        // ---------------------------------------------------------------------

        double current_playback_rate = info.audio_playback_rate.load();

        if (!m_was_playing || m_last_playback_rate != current_playback_rate) {
            // 首次播放或变速，进行一次硬重置
            m_was_playing = true;
            m_last_playback_rate = current_playback_rate;
            info.logic_canvas_time = info.raw_audio_time_ms.load();
            m_clock_rate = current_playback_rate;  // 速率也立即重置
        }

        // 1. 计算当前“水箱”的水位 (即画布与音频的领先差距)
        //    目标时间 = 音频原始时间 + 目标缓冲
        double target_time_with_buffer =
            info.raw_audio_time_ms.load() + TARGET_BUFFER_MS;
        double buffer_error = target_time_with_buffer - info.logic_canvas_time;

        // 2. 根据“水位”误差，计算一个理想的目标速率
        //    这是一个P控制器，但它控制的是“速率”，而不是“位置”
        //    Kp值决定了修正的强度
        const double Kp = 0.001;
        double target_rate = current_playback_rate + (buffer_error * Kp);

        // 3. 对目标速率进行范围限制，防止过激的调整
        target_rate =
            std::clamp(target_rate, current_playback_rate - MAX_RATE_ADJUSTMENT,
                       current_playback_rate + MAX_RATE_ADJUSTMENT);

        // 4. 【关键】平滑地逼近目标速率
        //    我们不直接设置 m_clock_rate = target_rate
        //    而是每一帧都向它靠近一点点，这保证了速度变化的绝对平滑
        m_clock_rate += (target_rate - m_clock_rate) * RATE_SMOOTHING_FACTOR;

        // 5. 【无回弹前进】以当前平滑的速率，让时间前进
        //    smoothed_delta_ms 必须 > 0, m_clock_rate 必须 > 0
        //    因此 logic_canvas_time 永远只会增加，绝不回弹！
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
    // 积分模式
    // P: 比例增益，用于修正相位（时间）误差
    const double Kp = 0.0125;
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

    // 水箱模式
    // “水箱”的目标缓冲水平 (单位: ms)
    // 这意味着我们总是试图让画布时间领先音频时间 20ms
    // 这个缓冲可以吸收音频和渲染循环之间的抖动
    const double TARGET_BUFFER_MS = 20.0;

    // 修正速率的最大调整幅度 (例如, 1.0 +/- 5%)
    // 这防止了因为一次巨大的误差导致画布速度变得过快或过慢
    const double MAX_RATE_ADJUSTMENT = 0.05;

    // --- 状态变量 ---

    // 用于平滑速率变化的平滑因子
    const double RATE_SMOOTHING_FACTOR = 0.005;
};

#endif  // MMM_MAPCANVASCLOCK_HPP
