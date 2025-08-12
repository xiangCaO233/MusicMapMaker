#ifndef MMM_EFFECTLAYER_HPP
#define MMM_EFFECTLAYER_HPP

#include <layer/ILayer.hpp>

class EffectLayer : public ILayer {
   public:
    // 构造EffectLayer
    explicit EffectLayer(Renderer2D* renderer);
    // 析构EffectLayer
    ~EffectLayer() override;

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;
};

#endif  // MMM_EFFECTLAYER_HPP
