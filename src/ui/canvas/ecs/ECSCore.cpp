#include <ecs/ECSCore.hpp>

// 构造ECSCore
ECSCore::ECSCore() {}

// 析构ECSCore
ECSCore::~ECSCore() {}

// 获取ecs reg
entt::registry& ECSCore::ecs_registry() { return registry; }

// 更新map
void ECSCore::updateMap(MMap* mmap) { map = mmap; }
