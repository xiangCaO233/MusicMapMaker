#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <render/synchronize/tick/map/MapDataLoop.hpp>

// pretick事件(执行其他任务)
// 这里是同步的-时间太长会影响帧数
void MapDataLoop::pre_tickEvent() {
    // qDebug() << "pretick开始";
    // 先筛选可见物件
    auto map_layermgr = static_cast<MapLayerManager*>(manager().get());
    if (!map_layermgr) return;
    auto map = map_layermgr->map();
    auto mapinfo = static_cast<MapCanvasInfo*>(getinfo());
    if (!map) {
        return;
    }

    auto& editor_info = mapinfo->editorInfo;
    auto converter_manager = map_layermgr->get_time_converter_manager();
    // 初始化时间映射器 (自动构造/构造函数自动执行预计算)
    auto converter = converter_manager->getConverter(
        map->timing_set(), mapinfo->baseInfo, editor_info.scrollInfo, mapinfo,
        map->base_metadata().preference_bpm);
    auto pconverter = converter_manager->getConverter(
        map->timing_set(), mapinfo->baseInfo, editor_info.scrollInfo, mapinfo,
        map->base_metadata().preference_bpm, true);
    auto& ecore = map_layermgr->core();

    // auto& linearConverter =
    //     map_layermgr->get_time_converter_manager()->getConverterLinear(
    //         map->timing_set(), mapinfo->baseInfo, mapinfo,
    //         mapinfo->editorInfo.map->base_metadata().preference_bpm);
    // auto& effectedConverter =
    //     map_layermgr->get_time_converter_manager()->getConverterEffected(
    //         map->timing_set(), mapinfo->baseInfo, mapinfo,
    //         mapinfo->editorInfo.map->base_metadata().preference_bpm);

    // qDebug() << "同步系统(at pretick)开始";
    // 同步编辑事件
    sync_system.updateEditStatus(ecore, map_layermgr);

    // 同步工具交互状态
    sync_system.updateToolInteractions(ecore, mapinfo, map_layermgr, *converter,
                                       *pconverter);

    // 与源物件集合同步可见的物件和timing
    sync_system.updateEntities(ecore, map->note_set(), map->note_uuids(),
                               map_layermgr, map->timing_set(),
                               map->beat_timeline(), map->beat_info(), mapinfo,
                               *converter
                               // , effectedConverter
    );
    // qDebug() << "时间转换系统(at pretick)开始";
    // 计算有时间属性的逻辑y轴位置
    time_system.update(ecore, mapinfo, *converter, *pconverter);
    // qDebug() << "时间转换系统(at pretick)结束";

    // 同步特效
    sync_system.updateEffects(ecore, map->note_set(), map->note_uuids(),
                              mapinfo, *converter);
    // qDebug() << "同步系统(at pretick)结束";

    // qDebug() << "pretick结束";
}

void MapDataLoop::tickEvent() {}

void MapDataLoop::after_tickEvent() {
    auto mapinfo = static_cast<MapCanvasInfo*>(getinfo());
    mapinfo->realTimeInfo.last_time_info.logic_canvas_time =
        mapinfo->realTimeInfo.current_time_info.logic_canvas_time;
    mapinfo->realTimeInfo.last_time_info.presentation_canvas_time =
        mapinfo->realTimeInfo.current_time_info.presentation_canvas_time;
    mapinfo->realTimeInfo.last_time_info.raw_audio_time_ms.store(
        mapinfo->realTimeInfo.current_time_info.raw_audio_time_ms.load());
}

// 初始化层管理器
void MapDataLoop::initializeLayerManager() {
    manager() = std::make_unique<MapLayerManager>(renderer());
    manager()->initializeLayers();
}
