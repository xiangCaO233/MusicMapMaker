#ifndef MMM_STATECOMPONENTS_HPP
#define MMM_STATECOMPONENTS_HPP

// 标记实体当前在屏幕内，应被渲染
struct VisibleComponent {};

// 标记实体在编辑器中被悬浮
struct HoveredComponent {};

// 标记实体在编辑器中被选中
struct SelectedComponent {};

#endif  // MMM_STATECOMPONENTS_HPP
