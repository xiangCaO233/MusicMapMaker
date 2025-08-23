#include <ecs/ECSCore.hpp>

// 构造ECSCore
ECSCore::ECSCore() = default;

// 析构ECSCore
ECSCore::~ECSCore() = default;

// 获取ecs reg
entt::registry& ECSCore::ecs_registry() { return registry; }

std::unordered_map<NoteHandle, entt::entity, NoteHandle::Hash>&
ECSCore::handle_to_entity_map() {
    return handle_to_entity;
}
std::unordered_map<TimingHandle, entt::entity, TimingHandle::Hash>&
ECSCore::handle_to_timingentity_map() {
    return timing_handle_to_entity_map;
}

// 更新map
void ECSCore::updateMap(MMap* mmap) { map = mmap; }
