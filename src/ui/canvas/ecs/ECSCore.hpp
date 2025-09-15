#ifndef MMM_ECSCORE_HPP
#define MMM_ECSCORE_HPP

#include <entt.hpp>
#include <mmm/ObjectHandle.hpp>
#include <mmm/timing/Beat.hpp>

class MMap;
class Note;

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>

class ECSCore {
   public:
    // 构造ECSCore
    ECSCore();

    // 析构ECSCore
    virtual ~ECSCore();

    // 获取ecs reg
    entt::registry& ecs_registry();
    std::unordered_map<NoteUUID, entt::entity>& uuid_to_entity_map();

    std::unordered_map<TimingHandle, entt::entity, TimingHandle::Hash>&
    handle_to_timingentity_map();

    std::unordered_map<BeatHandle, entt::entity>& handle_to_beatentity_map();

    // 更新map
    void updateMap(MMap* mmap);

    // auto beat_group() { return registry.group<TimeComponent,
    // BeatComponent>(); }
    // 提供访问器
    auto& get_beat_group() { return beat_group; }
    const auto& get_beat_group() const { return beat_group; }

   private:
    // map引用
    MMap* map{nullptr};

    // 持有所有实体(entities)和组件(components)
    entt::registry registry;

    // 拥有 TimeComponent 和 BeatComponent 的 group
    decltype(registry.group<TimeComponent, BeatComponent>()) beat_group;

    // NoteHandle -> entt::entity 的映射
    std::unordered_map<NoteUUID, entt::entity> uudi_to_entity;
    // TimingHandle -> entt::entity 的映射
    std::unordered_map<TimingHandle, entt::entity, TimingHandle::Hash>
        timing_handle_to_entity_map;
    // BeatHandle -> entt::entity 的映射
    std::unordered_map<BeatHandle, entt::entity> beat_handle_to_entity_map;
};
#endif  // MMM_ECSCORE_HPP
