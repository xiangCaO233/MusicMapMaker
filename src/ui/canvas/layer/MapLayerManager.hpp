#ifndef MMM_MAPLAYERMANAGER_HPP
#define MMM_MAPLAYERMANAGER_HPP

#include <layer/LayerManager.hpp>

class MapLayerManager : public LayerManager {
   public:
    // 构造MapLayerManager
    using LayerManager::LayerManager;

    // 析构MapLayerManager
    ~MapLayerManager() override;

    // 更新map
    void updateMap(MMap* map) override;

    // 初始化图层
    void initializeLayers() override;

    // 获取map引用
    MMap* map() const { return mapref; }

    // 获取ecs核心
    ECSCore& core() { return map_ecs_core; }

   private:
    // 核心ecs
    ECSCore map_ecs_core;

    // map引用
    MMap* mapref{nullptr};
};

#endif  // MMM_MAPLAYERMANAGER_HPP
