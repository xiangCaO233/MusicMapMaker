#include <layer/preview/PreviewLayer.hpp>

PreviewLayer::PreviewLayer(Renderer2D* renderer, ECSCore* ecore)
    : ILayer(renderer, ecore) {
    setType(LayerType::PREVIEW);
}

PreviewLayer::~PreviewLayer() {}
