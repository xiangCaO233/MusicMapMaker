#include <QDebug>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/note/NoteLayer.hpp>
#include <layer/note/NoteLayerGenerator.hpp>

// 析构NoteLayerGenerator
NoteLayerGenerator::~NoteLayerGenerator() {
    qDebug() << "物件图层生成线程释放";
}

// 生成交互层的数据
void NoteLayerGenerator::generateLayer(LayerManager* manager,
                                       ILayer::RenderDataBuffer& buffer) {
    // 数据准备
    auto maplayer_manager = static_cast<MapLayerManager*>(manager);
    auto map = maplayer_manager->map();
    if (!map) return;
    auto l = layer<NoteLayer>();
    auto mapinfo = static_cast<MapCanvasInfo*>(l->info());
    // 初始化时间映射器
    TimePixelConverter converter(map->timing_set(), mapinfo->baseInfo,
                                 map->base_metadata().preference_bpm);
    // 更新物件逻辑位置
    time_system.update(maplayer_manager->core(), mapinfo, converter);
}
