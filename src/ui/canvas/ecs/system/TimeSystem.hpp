#ifndef MMM_TIMESYSTEM_HPP
#define MMM_TIMESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

class TimeSystem {
   public:
    // 根据时间组件 附加 TransformComponent_1属性组件(仅计算出y位置)
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;

        // 遍历所有需要定位的实体
        auto view = registry.view<TimeComponent>();
        // auto count{0};
        for (auto entity : view) {
            auto [time] = view.get<TimeComponent>(entity);

            // 使用转换器计算Y坐标
            const float y = converter.timeToPixel(
                time, realtime_info.current_time_info.presentation_canvas_time,
                info);

            // 附加或更新 TransformComponent
            registry.emplace_or_replace<TransformComponent>(entity, y);
            // ++count;
        }
        // qDebug() << "共更新" << view->size() << "个实体时间转换组件";
    }
};

#endif  // MMM_TIMESYSTEM_HPP
