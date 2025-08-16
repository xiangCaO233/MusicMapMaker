#ifndef MMM_REALTIMEINTERACTLAYER_HPP
#define MMM_REALTIMEINTERACTLAYER_HPP

#include <QPoint>
#include <layer/ILayer.hpp>
#include <tool/BaseTool.hpp>

// 实时交互图层

class RealTimeInteractLayer : public ILayer {
   public:
    RealTimeInteractLayer(Renderer2D* renderer, ECSCore* ecore);
    ~RealTimeInteractLayer() override;
};

#endif  // MMM_REALTIMEINTERACTLAYER_HPP
