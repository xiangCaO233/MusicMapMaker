#ifndef MMM_TOOLINTERACTIONSTATE_HPP
#define MMM_TOOLINTERACTIONSTATE_HPP

#include <qnamespace.h>

#include <entt.hpp>
#include <glm/glm.hpp>
#include <info/NotePart.hpp>
#include <vector>

// --- 子结构：鼠标状态 ---
struct MouseState {
    // 当前光标位置 (高频更新)
    glm::vec2 current_pos;
    // 上次按下的位置
    glm::vec2 press_pos;
    // 鼠标轨迹 (用于特效)
    std::vector<glm::vec2> trail;
    // 当前按下的按钮
    QFlags<Qt::MouseButton> pressed_buttons;
};

// --- 子结构：选择状态 ---
struct SelectionState {
    std::unordered_set<entt::entity> selected_entities;
};

// --- 子结构：拖拽状态 ---
enum class DragMode {
    // 无拖动内容:生成选择框
    None,
    // 拖拽单个实体 (或多个已选实体)
    Entity,
    // 拖拽选择框
    Marquee,
};

struct DragState {
    // 拖拽模式
    DragMode mode{DragMode::None};
    // 拖拽操作是否有效
    bool is_valid{true};

    // 仅在 mode == Entity 时有效
    // 拖拽开始时的命中信息 (部位、实体等)
    MeshPartInfo drag_start_hit;
    // 实际被拖拽的实体集合
    std::unordered_set<entt::entity> dragged_entities;
};

// 主结构ToolInteractionState
class ToolInteractionState {
   public:
    // 构造函数
    ToolInteractionState() = default;

    // 线程安全的公共接口

    // 鼠标相关 (由UI线程写入, 所有线程读取)
    void updateMouse(const glm::vec2& pos, QFlags<Qt::MouseButton> buttons);
    MouseState getMouseState() const;

    // 悬浮相关 (由 pretick 写入, 工作线程读取)
    void setHover(const std::optional<MeshPartInfo> hover);
    std::optional<MeshPartInfo> getHover() const;

    // 拖拽相关 (由 pretick 写入, 工作线程读取)
    void startDrag(DragMode mode, MeshPartInfo hit,
                   const std::unordered_set<entt::entity>& selection);
    void endDrag();
    void setDragValidity(bool isValid);
    DragState getDragState() const;

    // 选择相关 (由 pretick 写入, 所有线程读取)
    void setSelection(const std::unordered_set<entt::entity>& entities);
    std::unordered_set<entt::entity> getSelection() const;

    // 操作/快捷键相关 (由UI/Action系统写入, pretick读取)
    // 这个可以用一个更简单的命令队列，或者一个原子标志位
    // void triggerAction(ActionType action);

   private:
    // 使用一个互斥锁保护所有状态的读写，确保一致性
    // 对于高频读写的场景，可以考虑为不同子结构使用不同的锁，或读写锁
    mutable std::mutex m_mutex;

    // 私有状态变量
    MouseState m_mouseState;
    // 当前悬浮的对象
    std::optional<MeshPartInfo> m_hovered;
    DragState m_dragState;
    SelectionState m_selectionState;
};

#endif  // MMM_TOOLINTERACTIONSTATE_HPP
