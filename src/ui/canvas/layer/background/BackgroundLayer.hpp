#ifndef MMM_BACKGROUNDLAYER_HPP
#define MMM_BACKGROUNDLAYER_HPP

#include <glm/fwd.hpp>
#include <layer/ILayer.hpp>

class BackgroundLayer : public ILayer {
   public:
    // 构造BackgroundLayer
    BackgroundLayer(Renderer2D* renderer, ECSCore* ecore);
    // 析构BackgroundLayer
    ~BackgroundLayer() override;

    friend class BackgroundLayerGenerator;
};
#endif  // MMM_BACKGROUNDLAYER_HPP
