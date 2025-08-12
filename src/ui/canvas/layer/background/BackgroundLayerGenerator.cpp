#include <QDebug>
#include <layer/background/BackgroundLayerGenerator.hpp>

// 析构BackgroundLayerGenerator
BackgroundLayerGenerator::~BackgroundLayerGenerator() {
    qDebug() << "背景图层生成线程释放";
}

// 生成图层
void BackgroundLayerGenerator::generateLayer(ILayer::RenderDataBuffer& buffer) {
}
