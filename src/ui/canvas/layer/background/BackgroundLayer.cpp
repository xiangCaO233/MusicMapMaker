#include <info/MapCanvasInfo.hpp>
#include <layer/background/BackgroundLayer.hpp>

// 构造BackgroundLayer
BackgroundLayer::BackgroundLayer(Renderer2D* renderer) : ILayer(renderer) {
    setType(LayerType::BACKGROUND);
}

// 析构BackgroundLayer
BackgroundLayer::~BackgroundLayer() {}

// 更新信息
void BackgroundLayer::updateInfo(SharedCanvasInfo* info) {
    auto mapinfo = static_cast<MapCanvasInfo*>(info);
    canvas_size = {info->baseInfo.canvasSize.width(),
                   info->baseInfo.canvasSize.height()};
    background_image_path = mapinfo->mapInfo.cover_path;
    texinfo = textureInfo(background_image_path);
}
