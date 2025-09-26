#include <log/colorful-log.h>

#include <QDebug>
#include <glm/gtc/constants.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/interact/InteractLayerGenerator.hpp>
#include <layer/interact/RealTimeInteractLayer.hpp>

// 析构InteractLayerGenerator
InteractLayerGenerator::~InteractLayerGenerator() {
    XINFO("交互图层生成线程释放");
}

// 生成交互层的数据
void InteractLayerGenerator::generateLayer(LayerManager* manager,
                                           RenderDataBuffer& buffer) {
    // qDebug() << "生成交互图层";
    auto l = layer<RealTimeInteractLayer>();
    auto mapinfo = static_cast<MapCanvasInfo*>(l->info());
    auto maplayer_manager = static_cast<MapLayerManager*>(manager);
    auto map = maplayer_manager->map();
    if (!map) return;
    auto converter =
        maplayer_manager->get_time_converter_manager()->getConverter(
            map->timing_set(), mapinfo->baseInfo,
            mapinfo->editorInfo.scrollInfo, mapinfo,
            mapinfo->editorInfo.map->base_metadata().preference_bpm);
    auto& ecore = maplayer_manager->core();

    interact_system.update(ecore, mapinfo, l, *converter,
                           tool_interaction_state, buffer);
    // qDebug() << "interact layer done";
}
