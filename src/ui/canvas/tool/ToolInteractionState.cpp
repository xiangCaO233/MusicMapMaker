#include <tool/ToolInteractionState.hpp>
// 线程安全的公共接口

// 鼠标相关 (由UI线程写入, 所有线程读取)
void ToolInteractionState::updateMouse(const glm::vec2& pos,
                                       QFlags<Qt::MouseButton> buttons) {
    m_mouseState.pressed_buttons = buttons;
    m_mouseState.current_pos = pos;
}

MouseState ToolInteractionState::getMouseState() const { return m_mouseState; }

// 悬浮相关 (由 pretick 写入, 工作线程读取)
void ToolInteractionState::setHover(
    const std::optional<const MeshPartInfo*>& hover) {
    m_hovered = hover;
}

std::optional<const MeshPartInfo*> ToolInteractionState::getHover() const {
    return m_hovered;
}

// 拖拽相关 (由 pretick 写入, 工作线程读取)
void ToolInteractionState::startDrag(
    DragMode mode, const MeshPartInfo& hit,
    const std::unordered_set<entt::entity>& selection) {
    m_dragState.drag_start_hit = &hit;
    if (hit.part != NotePart::NONE) {
        m_dragState.mode = DragMode::Entity;
    } else {
        m_dragState.mode = DragMode::Marquee;
    }
    m_dragState.dragged_entities = selection;
}

void ToolInteractionState::endDrag() {}

void ToolInteractionState::setDragValidity(bool isValid) {}

DragState ToolInteractionState::getDragState() const { return m_dragState; }

// 选择相关 (由 pretick 写入, 所有线程读取)
void ToolInteractionState::setSelection(
    const std::unordered_set<entt::entity>& entities) {
    m_selectionState.selected_entities = entities;
}

std::unordered_set<entt::entity> ToolInteractionState::getSelection() const {
    return m_selectionState.selected_entities;
}

// 操作/快捷键相关 (由UI/Action系统写入, pretick读取)
// 这个可以用一个更简单的命令队列，或者一个原子标志位
// void triggerAction(ActionType action);
