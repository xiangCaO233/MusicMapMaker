#include <ecs/system/TimePixelConverter.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <render/synchronize/tick/map/MapDataLoop.hpp>

// 可重写的tick事件(执行其他任务)
// 这里是同步的-时间太会影响帧数
void MapDataLoop::pre_tickEvent() {
    // 先筛选可见物件
    auto map_layermgr = static_cast<MapLayerManager *>(manager().get());
    auto map = map_layermgr->map();
    auto mapinfo = static_cast<MapCanvasInfo *>(getinfo());
    if (!map) return;

    // 初始化时间映射器 (自动构造/构造函数自动执行预计算)
    auto converter = map_layermgr->get_time_converter_manager()->getConverter(
        map->timing_set(), mapinfo->baseInfo,
        mapinfo->editorInfo.map->base_metadata().preference_bpm);
    auto &ecore = map_layermgr->core();

    // 与源物件集合同步可见的物件和timing
    sync_system.update(ecore, map->note_set(), map->timing_set(),
                       map->beat_timeline(), map->beat_info(), mapinfo,
                       converter);
    // 计算有时间属性的逻辑y轴位置
    time_system.update(ecore, mapinfo, converter);
}

void MapDataLoop::tickEvent() {}

void MapDataLoop::after_tickEvent() {}

// 初始化层管理器
void MapDataLoop::initializeLayerManager() {
    manager() = std::make_unique<MapLayerManager>(renderer());
    manager()->initializeLayers();
}
