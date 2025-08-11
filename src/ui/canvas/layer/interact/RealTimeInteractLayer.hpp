#ifndef MMM_REALTIMEINTERACTLAYER_HPP
#define MMM_REALTIMEINTERACTLAYER_HPP

#include <QPoint>
#include <layer/ILayer.hpp>
#include <tool/BaseTool.hpp>

// 实时交互图层

class RealTimeInteractLayer : public ILayer {
   public:
    RealTimeInteractLayer();
    ~RealTimeInteractLayer() override;

    // 鼠标当前位置
    QPointF mouse{0, 0};

    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;

    // 活跃工具
    BaseTool* activateTool;
};

#endif  // MMM_REALTIMEINTERACTLAYER_HPP
