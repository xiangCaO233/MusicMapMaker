#include <layer/effect/EffectLayer.hpp>

// 构造EffectLayer
EffectLayer::EffectLayer(Renderer2D* renderer) : ILayer(renderer) {
    setType(LayerType::EFFECT);
}

// 析构EffectLayer
EffectLayer::~EffectLayer() {}

// 更新信息
void EffectLayer::updateInfo(SharedCanvasInfo* info) {}
