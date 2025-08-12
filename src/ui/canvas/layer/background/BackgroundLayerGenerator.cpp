#include <QDebug>
#include <layer/background/BackgroundLayer.hpp>
#include <layer/background/BackgroundLayerGenerator.hpp>
#include <render/RenderCommand.hpp>

// 析构BackgroundLayerGenerator
BackgroundLayerGenerator::~BackgroundLayerGenerator() {
    qDebug() << "背景图层生成线程释放";
}

// 生成图层
void BackgroundLayerGenerator::generateLayer(ILayer::RenderDataBuffer& buffer) {
    auto bglayer = layer<BackgroundLayer>();
    if (!bglayer->background_image_path.empty()) {
        RenderCommand cmd;

        cmd.baseInfo = {{0.f, 0.f}, bglayer->canvas_size, 0.f};
        cmd.texturesInfo.texture = bglayer->texinfo;
        // cmd.texturesInfo.tscale = TexScaleMode::FORCE_FILL;
        // cmd.radiusInfo.radius = {.2f, .2f};
        // cmd.radiusInfo.radius_effect_param = {0.f};

        buffer.push_back(cmd);
    }
}
