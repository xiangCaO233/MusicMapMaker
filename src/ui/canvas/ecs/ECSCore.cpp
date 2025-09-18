#include <ecs/ECSCore.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <mmm/map/MMap.hpp>

// 构造ECSCore
ECSCore::ECSCore()
    : beat_group(registry.group<TimeComponent, BeatComponent>()) {};

// 析构ECSCore
ECSCore::~ECSCore() = default;

// 获取ecs reg
entt::registry& ECSCore::ecs_registry() { return registry; }

std::unordered_map<NoteUUID, entt::entity>& ECSCore::uuid_to_entity_map() {
    return uuid_to_entity;
}
std::unordered_map<TimingHandle, entt::entity, TimingHandle::Hash>&
ECSCore::handle_to_timingentity_map() {
    return timing_handle_to_entity_map;
}
std::unordered_map<BeatHandle, entt::entity>&
ECSCore::handle_to_beatentity_map() {
    return beat_handle_to_entity_map;
}

// 更新map
void ECSCore::updateMap(MMap* mmap) {
    map = mmap;
    // 更新特效实体
    // 清理可能残留的旧轨道特效
    registry.clear<EffectComponent>();

    for (int i = 0; i < map->base_metadata().track_count; ++i) {
        auto entity = registry.create();
        // 初始化时，特效处于“静默”状态
        // 我们可以用一个特殊的 texture_type 或无效的 last_reset_time 来表示
        registry.emplace<EffectComponent>(entity, i, EffectTextureType::NONE);
        // 附加一个轨道标签，便于查找
        registry.emplace<TrackIdentifierComponent>(entity, i);

        // 附加一个音效标签
        registry.emplace<SoundStateComponent>(entity);
    }
}
