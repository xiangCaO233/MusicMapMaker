#ifndef MMM_TIMELINELAYER_HPP
#define MMM_TIMELINELAYER_HPP

#include <layer/ILayer.hpp>

class TimelineLayer : public ILayer {
   public:
    // 构造TimelineLayer
    TimelineLayer(Renderer2D* renderer, ECSCore* ecore);
    // 析构TimelineLayer
    ~TimelineLayer() override;
};
#endif  // MMM_TIMELINELAYER_HPP
