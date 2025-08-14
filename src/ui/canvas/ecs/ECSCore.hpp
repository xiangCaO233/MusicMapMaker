#ifndef MMM_ECSCORE_HPP
#define MMM_ECSCORE_HPP

#include <entt.hpp>

class MMap;

class ECSCore {
   public:
    // 构造ECSCore
    ECSCore();
    // 析构ECSCore
    virtual ~ECSCore();

    // 获取ecs reg
    entt::registry& ecs_registry();

   private:
    MMap* map;
    // 持有所有实体(entities)和组件(components)
    entt::registry registry;
};
#endif  // MMM_ECSCORE_HPP
