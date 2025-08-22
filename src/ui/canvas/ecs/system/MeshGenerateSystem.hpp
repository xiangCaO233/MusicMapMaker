#ifndef MMM_MESHGENERATESYSTEM_HPP
#define MMM_MESHGENERATESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

class MeshGenerateSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter) {
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;
        // 遍历所有需要生成网格的实体
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent_1>();
        for (auto& e : view) {
            //
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
