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

    bool operator==(const BaseCanvasStatus& _) const = default;
};

struct RealTimeInfo {
    // 【呈现时间】最终用于渲染的时间戳
    double presentation_canvas_time{0.0};

    // 【逻辑时间】时钟内部使用的、与音频严格同步的时间
    // 这个时间对外部模块（如图层）是只读的，主要由时钟管理
    double logic_canvas_time{0.0};

    // --- 音频线程和主线程之间的同步点 ---
    // --- 音频/谱面固定偏移量 (ms) ---
    std::atomic<double> global_static_offset_ms{-110.0};

    // --- 音频/谱面全局偏移量 (ms) ---
    // 可以由UI控件修改，所以用原子保证线程安全
    std::atomic<double> global_offset_ms{0.0};

    // --- 音频线程提供的原始同步数据 ---
    // 音频播放器报告的、未经偏移修正的原始播放时间
    std::atomic<double> raw_audio_time_ms{0.0};

    // 音频播放器设定的播放速率 (1.0 = 正常, 0.5 = 半速, 2.0 = 倍速)
    // 同样，它可能由UI线程修改，所以使用原子类型
    std::atomic<double> audio_playback_rate{1.0};

    // --- 播放状态 ---
    bool is_playing{false};

    // 当前鼠标位置
    QPointF mousePos;
    // 正在按下的鼠标按钮
    std::unordered_set<Qt::MouseButton> mButtons;
};

struct SharedCanvasInfo {
    // 基本信息
    BaseCanvasStatus baseInfo;
    // 实时信息
    RealTimeInfo realTimeInfo;
};

#endif  // MMM_SHAREDCANVASINFO_HPP
