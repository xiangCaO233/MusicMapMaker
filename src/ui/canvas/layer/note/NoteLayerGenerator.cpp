#include <QDebug>
#include <ecs/component/TransformComponents.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/note/NoteLayer.hpp>
#include <layer/note/NoteLayerGenerator.hpp>

// 析构NoteLayerGenerator
NoteLayerGenerator::~NoteLayerGenerator() {
    qDebug() << "物件图层生成线程释放";
}

// 生成物件层的数据
void NoteLayerGenerator::generateLayer(LayerManager* manager,
                                       ILayer::RenderDataBuffer& buffer) {
    // 数据准备
    auto maplayer_manager = static_cast<MapLayerManager*>(manager);
    auto map = maplayer_manager->map();
    if (!map) return;
    auto l = layer<NoteLayer>();
    auto mapinfo = static_cast<MapCanvasInfo*>(l->info());
    auto& ecore = maplayer_manager->core();
    // 从管理器获取时间转换器
    auto converter =
        maplayer_manager->get_time_converter_manager()->getConverter(
            map->timing_set(), mapinfo->baseInfo,
            mapinfo->editorInfo.map->base_metadata().preference_bpm);

    // 生成物件网格
    mesh_system.update(ecore, mapinfo, converter);

    // 渲染一般可见物件
    normalRender_system.update(ecore, mapinfo, converter, buffer);
    // qDebug() << "note layer done";
}
