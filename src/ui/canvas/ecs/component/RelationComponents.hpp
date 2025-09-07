#ifndef MMM_RELATIONCOMPONENTS_HPP
#define MMM_RELATIONCOMPONENTS_HPP

#include <entt.hpp>

// 用于Composite Note的根节点
struct CompositeRootComponent {
    // 存储子Note实体的ID
    std::vector<entt::entity> children;
};

// 用于Composite Note的子节点，指向其父节点
struct ChildOfComponent {
    entt::entity parent;
};

#endif  // MMM_RELATIONCOMPONENTS_HPP
