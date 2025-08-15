#ifndef MMM_ECSCORE_HPP
#define MMM_ECSCORE_HPP

#include <entt.hpp>
#include <mmm/NoteHandle.hpp>

class MMap;
class Note;

class ECSCore {
   public:
    // 构造ECSCore
    ECSCore();

    // 析构ECSCore
    virtual ~ECSCore();

    // 获取ecs reg
    entt::registry& ecs_registry();
    std::unordered_map<NoteHandle, entt::entity, NoteHandle::Hash>&
    handle_to_entity_map();

    // 更新map
    void updateMap(MMap* mmap);

   private:
    // map引用
    MMap* map{nullptr};

    // 持有所有实体(entities)和组件(components)
    entt::registry registry;

    // NoteHandle -> entt::entity 的映射
    std::unordered_map<NoteHandle, entt::entity, NoteHandle::Hash>
        handle_to_entity;
};
#endif  // MMM_ECSCORE_HPP
