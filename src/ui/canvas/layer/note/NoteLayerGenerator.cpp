#include <QDebug>
#include <layer/note/NoteLayerGenerator.hpp>

// 析构NoteLayerGenerator
NoteLayerGenerator::~NoteLayerGenerator() {
    qDebug() << "物件图层生成线程释放";
}

// 生成交互层的数据
void NoteLayerGenerator::generateLayer(ILayer::RenderDataBuffer& buffer) {}
