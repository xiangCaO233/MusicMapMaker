#ifndef MMM_RELATIONCOMPONENTS_HPP
#define MMM_RELATIONCOMPONENTS_HPP

#include <entt.hpp>

// 用于Composite Note的根节点
struct CompositeRootComponent {
    // 存储子Note实体的ID
    std::vector<entt::entity> children;
};

// 用于Composite Note的子节点，指向其父节点
// 包含处于父物件集合中的索引
struct ChildOfComponent {
    entt::entity parent;
    size_t child_index;
};

#endif  // MMM_RELATIONCOMPONENTS_HPP
