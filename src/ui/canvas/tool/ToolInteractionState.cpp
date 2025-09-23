#include <QDebug>
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
void ToolInteractionState::setHover(const std::optional<MeshPartInfo> hover) {
    // if (hover.has_value()) {
    //     auto id = hover.value().handle.index;
    //     // qDebug() << "设置更新悬浮位置:" << id;
    // } else {
    //     // qDebug() << "清除悬浮位置";
    // }
    m_hovered = hover;
}

std::optional<MeshPartInfo> ToolInteractionState::getHover() const {
    return m_hovered;
}

// 拖拽相关 (由 pretick 写入, 工作线程读取)
void ToolInteractionState::startDrag(
    DragMode mode, MeshPartInfo hit,
    const std::unordered_set<entt::entity>& selection) {
    m_dragState.drag_start_hit = hit;
    m_dragState.dragged_entitiesWithRes.clear();
    for (auto e : selection) {
        m_dragState.dragged_entitiesWithRes.emplace(e, MapAxis{});
    }
    m_dragState.mode = mode;
}

void ToolInteractionState::endDrag() {
    m_dragState.mode = DragMode::None;
    m_dragState.drag_start_hit = {};
    m_dragState.dragged_entitiesWithRes.clear();
}

void ToolInteractionState::setDragValidity(bool isValid) {
    m_dragState.is_valid = isValid;
}

void ToolInteractionState::setDragValidRes(const entt::entity& e,
                                           const MapAxis& axis) {
    auto it = m_dragState.dragged_entitiesWithRes.find(e);
    if (it != m_dragState.dragged_entitiesWithRes.end()) {
        it->second = axis;
    }
}

DragState ToolInteractionState::getDragState() const { return m_dragState; }

// 创建相关(由 pretick 写入, 工作线程读取)
void ToolInteractionState::startCreate(CreateMode mode) {
    m_createState.mode = mode;
}

void ToolInteractionState::updateCreateNode(const MapAxis& axis) {
    if (m_createState.mode == CreateMode::Normal) {
        // 创建单键模式:直接替换创建结果，但只在变化时
        if (m_createState.createState_nodes.empty() ||
            m_createState.createState_nodes.back() != axis) {
            m_createState.createState_nodes.clear();
            m_createState.createState_nodes.push_back(axis);
        }
        return;
    } else if (m_createState.mode == CreateMode::Composite) {
        // 创建组合键模式:高频调用，过滤连续相同
        if (!m_createState.createState_nodes.empty() &&
            m_createState.createState_nodes.back() == axis) {
            // 相同则忽略，无操作
            return;
        }

        size_t size = m_createState.createState_nodes.size();
        if (size >= 2) {
            auto rit = m_createState.createState_nodes.rbegin();
            const auto& last = *rit;  // 倒数第一个
            ++rit;
            const auto& prev = *rit;  // 倒数第二个
            if (prev == axis) {  // 如果等于倒数第二个，则回退（pop最后一个）
                m_createState.createState_nodes.pop_back();
                return;
            }
        }

        // 否则，检查是否是延伸更新最后一个节点
        if (size >= 2) {
            auto rit = m_createState.createState_nodes.rbegin();
            const auto& last = *rit;  // 倒数第一个
            ++rit;
            const auto& prev = *rit;  // 倒数第二个
            bool prev_same_time = (prev.time == last.time);
            bool prev_same_track = (prev.track == last.track);
            bool prev_is_hold = prev_same_track && !prev_same_time;

            bool same_time_with_last = (last.time == axis.time);
            bool same_track_with_last = (last.track == axis.track);

            if (prev_is_hold && same_track_with_last && !same_time_with_last &&
                (axis.time - prev.time >= 0)) {
                // 延伸Hold：替换last为axis，但duration = axis.time - prev.time
                // >= 0
                m_createState.createState_nodes.back() = axis;
                return;
            } else if (!prev_is_hold && same_time_with_last &&
                       !same_track_with_last) {
                // 延伸Flick：same time, diff track (delta可负，无需检查)
                m_createState.createState_nodes.back() = axis;
                return;
            }
        }

        // 如果不是延伸，尝试添加新节点，检查合法
        if (size == 0) {
            // 空列表，直接加第一个
            m_createState.createState_nodes.push_back(axis);
            return;
        }

        // 检查与最后一个的相邻规则
        const auto& last = m_createState.createState_nodes.back();
        bool same_time = (last.time == axis.time);
        bool same_track = (last.track == axis.track);
        if (same_time == same_track) {  // 必须exactly one相同
            return;                     // 不添加
        }

        bool is_hold = same_track && !same_time;

        // 对于添加新Hold，检查duration >= 0，即 axis.time > last.time
        if (is_hold && axis.time <= last.time) {
            return;  // 不添加负duration
        }

        // 如果size == 1，任意（hold或flick）都行
        if (size == 1) {
            m_createState.createState_nodes.push_back(axis);
            return;
        }

        // size >= 2，检查交替
        // 前一个连接类型：检查倒数第二个和最后一个
        auto rit = m_createState.createState_nodes.rbegin();
        const auto& prev_last = *rit;  // 倒数第一个 (last)
        ++rit;
        const auto& prev = *rit;  // 倒数第二个
        bool prev_same_time = (prev.time == prev_last.time);
        bool prev_same_track = (prev.track == prev_last.track);
        bool prev_is_hold =
            prev_same_track && !prev_same_time;  // hold: same_track, diff time

        // 预期下一个连接：与prev相反
        bool expected_hold = !prev_is_hold;

        // 当前拟添加的连接类型
        bool current_is_hold = same_track && !same_time;

        if (current_is_hold == expected_hold) {
            m_createState.createState_nodes.push_back(axis);
        } else {
            // 不添加
        }
    }
}
void ToolInteractionState::setCreateValidity(bool isValid) {
    m_createState.is_valid = isValid;
}

CreateState ToolInteractionState::getCreateState() const {
    return m_createState;
}

void ToolInteractionState::endCreate() {
    m_createState.mode = CreateMode::None;
    m_createState.createState_nodes.clear();
}

// 删除相关
void ToolInteractionState::startDeleteCheck(
    const std::unordered_set<entt::entity>& selection) {
    m_deleteMarkState.marked_entities = selection;
}

void ToolInteractionState::endDeleteCheck() {
    m_deleteMarkState.marked_entities.clear();
}

DeleteMarkStates ToolInteractionState::getDeleteMarkStates() const {
    return m_deleteMarkState;
}

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
