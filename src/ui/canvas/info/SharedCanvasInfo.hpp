#ifndef MMM_SHAREDCANVASINFO_HPP
#define MMM_SHAREDCANVASINFO_HPP

#include <QPoint>
#include <QSize>
#include <unordered_set>

struct BaseCanvasStatus {
    // 画布尺寸
    QSizeF canvasSize;

    // 范围稍大以预加载(预留50ms)
    uint32_t view_timeMargin{100};

    // 滚动速度
    float scroll_speed{1.f};

    // 时间线缩放
    float timeline_zoom{1.f};

    // 判定线位置
    float judgeline_pos{.2f};
};

struct RealTimeInfo {
    // 当前时间戳
    double current_canvas_time{0};

    // --- 音频线程和主线程之间的同步点 ---
    // --- 音频/谱面全局偏移量 (ms) ---
    // 可以由UI控件修改，所以用原子保证线程安全
    std::atomic<double> global_offset_ms{0.0};

    // --- 音频线程提供的原始同步数据 ---
    // 音频播放器报告的、未经偏移修正的原始播放时间
    std::atomic<double> raw_audio_time_ms{0.0};

    // --- 播放状态 ---
    bool is_playing{false};

    // 当前鼠标位置
    QPointF mousePos;
    // 正在按下的鼠标按钮
    std::unordered_set<Qt::MouseButton> buttons;
};

struct SharedCanvasInfo {
    // 基本信息
    BaseCanvasStatus baseInfo;
    // 实时信息
    RealTimeInfo realTimeInfo;
};

#endif  // MMM_SHAREDCANVASINFO_HPP
