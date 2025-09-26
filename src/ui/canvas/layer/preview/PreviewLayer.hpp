#ifndef MMM_PREVIEWLAYER_HPP
#define MMM_PREVIEWLAYER_HPP

#include <layer/ILayer.hpp>

class PreviewLayer : public ILayer {
   public:
    // 构造PreviewLayer
    PreviewLayer(Renderer2D* renderer, ECSCore* ecore);
    // 析构PreviewLayer
    ~PreviewLayer() override;
};

#endif  // MMM_PREVIEWLAYER_HPP
