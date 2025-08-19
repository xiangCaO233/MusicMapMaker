#include <QDebug>
#include <layer/timeline/TimelineLayerGenerator.hpp>

// 析构TimelineLayerGenerator
TimelineLayerGenerator::~TimelineLayerGenerator() {
    qDebug() << "时间线图层生成线程释放";
}

// 生成交互层的数据
void TimelineLayerGenerator::generateLayer(LayerManager* manager,
                                           ILayer::RenderDataBuffer& buffer) {
    // qDebug() << "timeline layer done";
}
