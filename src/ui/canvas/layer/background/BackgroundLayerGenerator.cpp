#include <QDebug>
#include <info/MapCanvasInfo.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <layer/background/BackgroundLayer.hpp>
#include <layer/background/BackgroundLayerGenerator.hpp>
#include <render/RenderCommand.hpp>

// 析构BackgroundLayerGenerator
BackgroundLayerGenerator::~BackgroundLayerGenerator() {
    qDebug() << "背景图层生成线程释放";
}

// 生成图层
void BackgroundLayerGenerator::generateLayer(LayerManager* manager,
                                             ILayer::RenderDataBuffer& buffer) {
    auto bglayer = layer<BackgroundLayer>();
    auto info = static_cast<MapCanvasInfo*>(bglayer->info());
    glm::vec2 canvas_size = {info->baseInfo.canvasSize.width(),
                             info->baseInfo.canvasSize.height()};
    auto background_image_path = info->mapInfo.cover_path;
    auto darken = info->mapInfo.darken;
    auto alpha = info->mapInfo.alpha;
    auto texinfo = bglayer->textureInfo(background_image_path);

    if (!background_image_path.empty()) {
        RenderCommand cmd;

        cmd.baseInfo = {{0.f, 0.f}, canvas_size, 0.f};
        cmd.baseInfo.color = {darken, darken, darken, alpha};
        cmd.texturesInfo.texture = texinfo;

        buffer.push_back(cmd);
    }
}
