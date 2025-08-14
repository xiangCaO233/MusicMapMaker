#include <info/SharedCanvasInfo.hpp>
#include <layer/interact/RealTimeInteractLayer.hpp>

// 构造实时交互图层
RealTimeInteractLayer::RealTimeInteractLayer(Renderer2D* renderer,
                                             ECSCore* ecore)
    : ILayer(renderer, ecore) {
    setType(LayerType::INTERACT);
}

// 析构实时交互图层
RealTimeInteractLayer::~RealTimeInteractLayer() = default;

// 更新信息
void RealTimeInteractLayer::updateInfo(SharedCanvasInfo* info) {
    mouse = info->realTimeInfo.mousePos;
    pressed = info->realTimeInfo.buttons.contains(Qt::LeftButton);
}
