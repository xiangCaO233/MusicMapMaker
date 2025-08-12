#ifndef MMM_BACKGROUNDLAYER_HPP
#define MMM_BACKGROUNDLAYER_HPP

#include <layer/ILayer.hpp>

class BackgroundLayer : public ILayer {
   public:
    // 构造BackgroundLayer
    BackgroundLayer();
    // 析构BackgroundLayer
    ~BackgroundLayer() override;

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;
};
#endif  // MMM_BACKGROUNDLAYER_HPP
