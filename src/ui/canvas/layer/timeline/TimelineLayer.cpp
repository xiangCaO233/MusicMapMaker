#include <layer/timeline/TimelineLayer.hpp>

// 构造TimelineLayer
TimelineLayer::TimelineLayer(Renderer2D* renderer) : ILayer(renderer) {
    setType(LayerType::TIMELINE);
}

// 析构TimelineLayer
TimelineLayer::~TimelineLayer() {}

// 更新信息
void TimelineLayer::updateInfo(SharedCanvasInfo* info) {}
