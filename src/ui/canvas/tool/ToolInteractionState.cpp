
#include <QDebug>
#include <tool/ToolInteractionState.hpp>
// 线程安全的公共接口

// 鼠标相关 (由UI线程写入, 所有线程读取)
// 鼠标相关 (由UI线程写入, 所有线程读取)
void ToolInteractionState::updateMousePress(
    const glm::vec2& pos, Qt::MouseButton button,
    QFlags<Qt::MouseButton> allButtons) {
    // 使用写入锁，确保在更新期间没有其他线程可以读取
    const QWriteLocker locker(&m_mouseStateLock);
    m_mouseState.current_pos = pos;
    m_mouseState.pressed_buttons = allButtons;
    m_mouseState.press_pos[button] = pos;  // 记录按下位置
}

void ToolInteractionState::updateMouseMove(const glm::vec2& pos,
                                           QFlags<Qt::MouseButton> allButtons) {
    // 使用写入锁，确保在更新期间没有其他线程可以读取
    const QWriteLocker locker(&m_mouseStateLock);
    m_mouseState.current_pos = pos;
    m_mouseState.pressed_buttons = allButtons;  // 移动时也可能伴随按键状态变化

    // 添加到轨迹
    m_mouseState.trail.push_back(pos);

    // 可选：为了防止轨迹无限增长，可以限制其大小
    // constexpr size_t MAX_TRAIL_SIZE = 100;
    // if (m_mouseState.trail.size() > MAX_TRAIL_SIZE) {
    //     m_mouseState.trail.erase(m_mouseState.trail.begin());
    // }
}

void ToolInteractionState::updateMouseRelease(
    const glm::vec2& pos, Qt::MouseButton button,
    QFlags<Qt::MouseButton> allButtons) {
    // 使用写入锁，确保在更新期间没有其他线程可以读取
    const QWriteLocker locker(&m_mouseStateLock);
    m_mouseState.current_pos = pos;
    m_mouseState.pressed_buttons = allButtons;
    m_mouseState.press_pos.erase(button);  // 移除释放按钮的按下记录
}

MouseState ToolInteractionState::getMouseState() const {
    // 使用读取锁，允许多个读取者并发
    const QReadLocker locker(&m_mouseStateLock);
    return m_mouseState;  // 返回一个深拷贝
}

// 如果只需要某个字段，可以提供专门的getter以提高效率
glm::vec2 ToolInteractionState::getCurrentMousePos() const {
    const QReadLocker locker(&m_mouseStateLock);
    return m_mouseState.current_pos;
}

glm::vec2 ToolInteractionState::getMousePressPos(
    const Qt::MouseButton button) const {
    const QReadLocker locker(&m_mouseStateLock);
    auto it = m_mouseState.press_pos.find(button);
    if (it != m_mouseState.press_pos.end()) {
        return it->second;
    }
    return glm::vec2{0};
}

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
    const std::unordered_map<entt::entity, MapAxis>& selection) {
    m_dragState.drag_start_hit = hit;
    m_dragState.dragged_entitiesWithRes = selection;
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
void ToolInteractionState::startNewSelectArea(
    bool append, Qt::MouseButton button, const glm::vec2& start_absolute_pos,
    const glm::vec2& start_time_pos) {
    // [关键逻辑] 如果不是追加模式，则只清空“当前”按钮的专属区域列表
    // 其他按钮（如图层）的选择区域将保持不变。
    if (!append) {
        m_selectionState.per_button_absolute_areas[button].clear();
        m_selectionState.per_button_areas[button].clear();
    }

    // 获取当前按钮的区域列表（如果不存在，map 会自动创建）
    auto& button_specific_abs_areas =
        m_selectionState.per_button_absolute_areas[button];
    auto& button_specific_time_areas =
        m_selectionState.per_button_areas[button];

    // 在这个专属列表中添加一个新的选择框
    button_specific_abs_areas.emplace_back(glm::vec4(start_absolute_pos, 0, 0));
    button_specific_time_areas.emplace_back(glm::vec4(start_time_pos, 0, 0));
    const size_t new_area_index = button_specific_abs_areas.size() - 1;

    // 创建活动会话，记录它正在更新其专属列表中的最后一个元素
    m_selectionState.active_sessions[button] = {start_absolute_pos,
                                                start_time_pos, new_area_index};
}

void ToolInteractionState::updateSelectArea(
    QFlags<Qt::MouseButton> current_buttons,
    const TimePixelConverter& converter, float canvas_height,
    float judgeline_abspos, float current_canvas_time) {
    // 遍历所有当前正在拖动的会话
    for (auto const& [button, session] : m_selectionState.active_sessions) {
        // 检查这个会话对应的按钮是否仍被按下
        if (current_buttons.testFlag(button)) {
            // 定位到该按钮的专属区域列表
            auto& button_specific_abs_areas =
                m_selectionState.per_button_absolute_areas[button];
            auto& button_specific_time_areas =
                m_selectionState.per_button_areas[button];
            // 从列表中找到并更新该会话对应的那个选择框
            auto& abs_area_to_update =
                button_specific_abs_areas[session.area_index_in_button_vector];
            auto& time_area_to_update =
                button_specific_time_areas[session.area_index_in_button_vector];

            abs_area_to_update = {
                session.start_absolute_pos,
                m_mouseState.current_pos - session.start_absolute_pos};

            auto width = m_mouseState.current_pos.x - session.start_pos.x;

            auto current_mouse_time = converter.distanceToTime(
                canvas_height - m_mouseState.current_pos.y - judgeline_abspos,
                current_canvas_time);
            auto duration = current_mouse_time - session.start_pos.y;
            time_area_to_update = {session.start_pos,
                                   glm::vec2{width, duration}};
            // 先构建格式化字符串
            // QString log_message =
            //     QString("[xstart:%1,timestart:%2,width:%3,duration:%4]")
            //         .arg(time_area_to_update.x, 0, 'f', 2)
            //         .arg(time_area_to_update.y, 0, 'f', 2)
            //         .arg(time_area_to_update.z, 0, 'f', 2)
            //         .arg(time_area_to_update.w, 0, 'f', 2);
            // qDebug() << "current mouse y:" << m_mouseState.current_pos.y;
            // qDebug() << "current mouse time:" << current_mouse_time;
            // // 然后将所有部分用 + 拼接成一个 QString
            // qDebug().noquote()
            //     << QString("button: %1 's time area update to %2")
            //            .arg(
            //                static_cast<int>(button))  // 将 QFlags 转为 int
            //                打印
            //            .arg(log_message);
        }
    }
}

void ToolInteractionState::endNewSelectArea(Qt::MouseButton released_button) {
    // 当一个按钮被释放，只从活动会话中移除它
    // 它所创建的选择框被保留下来，但不再更新
    m_selectionState.active_sessions.erase(released_button);
}

void ToolInteractionState::setSelection(
    Qt::MouseButton button, const std::unordered_set<entt::entity>& entities) {
    m_selectionState.all_selected_entities[button] = entities;
    rebuildAggregatedSelection();
}

std::unordered_set<entt::entity> ToolInteractionState::getSelection(
    Qt::MouseButton button) {
    return m_selectionState.all_selected_entities[button];
}

SelectionState ToolInteractionState::getSelectionState() const {
    return m_selectionState;
}

// 剪切板相关
void ToolInteractionState::setClipBoard(
    const std::unordered_set<NoteUUID>& uuids, bool is_copy) {
    m_noteClipboard.is_copy = is_copy;
    m_noteClipboard.uuids = uuids;
}

NoteClipboard ToolInteractionState::getClipBoard() const {
    return m_noteClipboard;
}

// 重建聚合选中区
void ToolInteractionState::rebuildAggregatedSelection() {
    m_aggregated_selection.clear();
    for (const auto& [button, selected_set] :
         m_selectionState.all_selected_entities) {
        m_aggregated_selection.insert(selected_set.begin(), selected_set.end());
    }
}

bool ToolInteractionState::isSelected(entt::entity entity_to_check) const {
    return m_aggregated_selection.contains(entity_to_check);
}

bool ToolInteractionState::hasSelected() const {
    return !m_aggregated_selection.empty();
}
