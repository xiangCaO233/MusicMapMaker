#include <layer/timeline/TimelineLayer.hpp>

// 构造TimelineLayer
TimelineLayer::TimelineLayer(Renderer2D* renderer, ECSCore* ecore)
    : ILayer(renderer, ecore) {
    setType(LayerType::TIMELINE);
}

// 析构TimelineLayer
TimelineLayer::~TimelineLayer() {}
