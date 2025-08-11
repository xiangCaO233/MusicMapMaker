#include <QDebug>
#include <layer/interact/InteractLayerGenerator.hpp>

// 析构InteractLayerGenerator
InteractLayerGenerator::~InteractLayerGenerator() {
    qDebug() << "交互图层生成线程释放";
}

// 生成交互层的数据
void InteractLayerGenerator::generateLayer(ILayer::RenderDataBuffer& buffer) {
    // qDebug() << "生成交互图层";
}
