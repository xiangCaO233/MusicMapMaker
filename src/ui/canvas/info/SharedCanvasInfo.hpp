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
    uint32_t current_canvas_time{2000};
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
