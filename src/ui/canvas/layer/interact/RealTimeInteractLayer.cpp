#include <info/SharedCanvasInfo.hpp>
#include <layer/interact/RealTimeInteractLayer.hpp>

// 构造实时交互图层
RealTimeInteractLayer::RealTimeInteractLayer() { setType(LayerType::INTERACT); }

// 析构实时交互图层
RealTimeInteractLayer::~RealTimeInteractLayer() = default;

// 更新信息
void RealTimeInteractLayer::updateInfo(SharedCanvasInfo* info) {
    mouse = info->realTimeInfo.mousePos;
}
