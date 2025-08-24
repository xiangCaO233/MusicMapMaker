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
    double current_canvas_time{92000};
    // --- 音频线程和主线程之间的同步点 ---
    // 使用原子变量来安全地跨线程传递最新的校准信息
    std::atomic<double> last_audio_time_ms{0.0};
    // 上次同步时，系统高精度时钟的时间点
    std::atomic<long long> last_sync_point_us{0};

    // 是否暂停
    bool pause{false};
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
