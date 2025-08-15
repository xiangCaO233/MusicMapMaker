#include <render/synchronize/tick/map/MapDataLoop.hpp>

#include "layer/MapLayerManager.hpp"

// 可重写的tick事件(执行其他任务)
void MapDataLoop::tickEvent() {}

// 初始化层管理器
void MapDataLoop::initializeLayerManager() {
    manager() = std::make_unique<MapLayerManager>(renderer());
    manager()->initializeLayers();
}
