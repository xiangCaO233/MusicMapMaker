#ifndef MMM_SHAREDCANVASINFO_HPP
#define MMM_SHAREDCANVASINFO_HPP

#include <qnamespace.h>

#include <QPoint>
#include <QSize>
#include <unordered_set>

struct BaseCanvasStatus {
    // 画布尺寸
    QSizeF canvasSize;
};

struct RealTimeInfo {
    // 当前时间戳
    uint32_t current_canvas_time;
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
