#include <layer/effect/EffectLayer.hpp>

// 构造EffectLayer
EffectLayer::EffectLayer() { setType(LayerType::EFFECT); }

// 析构EffectLayer
EffectLayer::~EffectLayer() {}

// 更新信息
void EffectLayer::updateInfo(SharedCanvasInfo* info) {}
