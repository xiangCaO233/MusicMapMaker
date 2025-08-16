#ifndef MMM_REALTIMEINTERACTLAYER_HPP
#define MMM_REALTIMEINTERACTLAYER_HPP

#include <layer/ILayer.hpp>

// 实时交互图层

class RealTimeInteractLayer : public ILayer {
   public:
    RealTimeInteractLayer(Renderer2D* renderer, ECSCore* ecore);
    ~RealTimeInteractLayer() override;
};

#endif  // MMM_REALTIMEINTERACTLAYER_HPP
