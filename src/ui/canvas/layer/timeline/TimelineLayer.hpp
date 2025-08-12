#ifndef MMM_TIMELINELAYER_HPP
#define MMM_TIMELINELAYER_HPP

#include <layer/ILayer.hpp>

class TimelineLayer : public ILayer {
   public:
    // 构造TimelineLayer
    TimelineLayer();
    // 析构TimelineLayer
    ~TimelineLayer() override;

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;
};
#endif  // MMM_TIMELINELAYER_HPP
