#ifndef MMM_TIMESYSTEM_HPP
#define MMM_TIMESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/NoteComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>

class TimeSystem {
   public:
    // 根据时间组件 附加 其他属性组件
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter) const {
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;

        // 1. 获取轨道布局信息
        // 假设 all_tracks_rect 定义了所有轨道占据的矩形区域 {x, y, width,
        // height}
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        const int track_count =
            info->editorInfo.map->base_metadata().track_count;
        if (track_count == 0) return;
        const float single_track_width = all_tracks_rect.z / float(track_count);

        // 2. 遍历所有需要定位的实体
        auto view = registry.view<NoteComponent>();
        for (auto entity : view) {
            auto [time, track, note] = view.get<NoteComponent>(entity);

            // 3. 使用转换器计算Y坐标
            const float y =
                converter.timeToPixel(time, realtime_info.current_canvas_time);

            // 4. 根据轨道号计算逻辑X坐标 (轨道的中心点)
            const float x =
                all_tracks_rect.x + (float(track) + 0.5f) * single_track_width;

            // 5. 附加或更新 TransformComponent_1
            registry.emplace_or_replace<TransformComponent_1>(entity, x, y);
        }
    }
};

#endif  // MMM_TIMESYSTEM_HPP
