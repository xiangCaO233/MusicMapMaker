#ifndef MMM_TIMESYSTEM_HPP
#define MMM_TIMESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

class TimeSystem {
   public:
    // 根据时间组件 附加 TransformComponent_1属性组件(仅计算出y位置)
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;

        // 判定线的绝对屏幕Y坐标
        const float judgeline_absolute_y = info->baseInfo.canvasSize.height() *
                                           (1.f - info->baseInfo.judgeline_pos);

        // 遍历所有需要定位的实体
        auto view = registry.view<TimeComponent>();
        // auto count{0};
        for (auto entity : view) {
            auto [time] = view.get<TimeComponent>(entity);

            // 使用转换器计算Y坐标
            const float y =
                converter.timeToPixel(time, realtime_info.current_canvas_time);

            // 判定线位置 + 逻辑偏移
            const float center_y = info->baseInfo.canvasSize.height() - y -
                                   (float(info->baseInfo.canvasSize.height()) -
                                    judgeline_absolute_y);

            // 附加或更新 TransformComponent_1
            registry.emplace_or_replace<TransformComponent_1>(entity, center_y);
            // ++count;
        }
        // qDebug() << "共更新" << count << "个实体转换组件1";
    }
};

#endif  // MMM_TIMESYSTEM_HPP
