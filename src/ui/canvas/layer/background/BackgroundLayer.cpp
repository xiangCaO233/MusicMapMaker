#include <info/MapCanvasInfo.hpp>
#include <layer/background/BackgroundLayer.hpp>

// 构造BackgroundLayer
BackgroundLayer::BackgroundLayer(Renderer2D* renderer, ECSCore* ecore)
    : ILayer(renderer, ecore) {
    setType(LayerType::BACKGROUND);
}

// 析构BackgroundLayer
BackgroundLayer::~BackgroundLayer() {}
