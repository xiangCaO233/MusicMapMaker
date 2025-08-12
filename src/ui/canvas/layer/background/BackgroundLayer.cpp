#include <layer/background/BackgroundLayer.hpp>

// 构造BackgroundLayer
BackgroundLayer::BackgroundLayer() { setType(LayerType::BACKGROUND); }

// 析构BackgroundLayer
BackgroundLayer::~BackgroundLayer() {}

// 更新信息
void BackgroundLayer::updateInfo(SharedCanvasInfo* info) {}
