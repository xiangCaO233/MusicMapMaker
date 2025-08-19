#include <QDebug>
#include <layer/effect/EffectLayerGenerator.hpp>

// 析构EffectLayerGenerator
EffectLayerGenerator::~EffectLayerGenerator() {
    qDebug() << "效果图层生成线程释放";
}

// 生成图层
void EffectLayerGenerator::generateLayer(LayerManager* manager,
                                         ILayer::RenderDataBuffer& buffer) {
    // qDebug() << "effect layer done";
}
