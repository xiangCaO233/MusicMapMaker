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
                const TimePixelConverter& converter) const {
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        // const auto& realtime_info = info->realTimeInfo;
        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        const int track_count =
            info->editorInfo.map->base_metadata().track_count;
        if (track_count == 0) return;
        const float single_track_width = all_tracks_rect.z / float(track_count);
        // 遍历所有需要生成网格的实体
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent_1>();
        for (auto& e : view) {
            // 清理上一帧的网格数据
            auto& mesh = registry.get_or_emplace<TransformComponent_2>(e).mesh;
            mesh.clear();
            // --- 根据Note类型进行分支处理 ---
            const auto& [track_index, handle] = registry.get<NoteComponent>(e);
            const auto& [y] = registry.get<TransformComponent_1>(e);
            const float x = all_tracks_rect.x +
                            (float(track_index) + 0.5f) * single_track_width;

            if (registry.all_of<HoldComponent>(e)) {
                mesh.emplace_back(
                    glm::vec2(x - (single_track_width / 2.f - 4.f), y - 40),
                    glm::vec2(single_track_width - 8.f, 80));
                // generateHoldMesh(registry, e);
            } else if (registry.all_of<FlickComponent>(e)) {
                mesh.emplace_back(
                    glm::vec2(x - (single_track_width / 2.f - 4.f), y - 15),
                    glm::vec2(single_track_width - 8.f, 30));
                // generateFlickMesh(registry, e);
            } else {
                mesh.emplace_back(
                    glm::vec2(x - (single_track_width / 2.f - 4.f), y - 15),
                    glm::vec2(single_track_width - 8.f, 30));
                // generateTapMesh(registry, e);
            }
        }
    }
};
#endif  // MMM_MESHGENERATESYSTEM_HPP
