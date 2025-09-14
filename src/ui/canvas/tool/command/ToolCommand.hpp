#ifndef MMM_TOOLCOMMAND_HPP
#define MMM_TOOLCOMMAND_HPP

#include <qnamespace.h>

#include <info/NotePart.hpp>
#include <variant>

struct DragStartInfo {
    glm::vec2 start_mouse_pos;
    QFlags<Qt::KeyboardModifier> modifiers;
};

struct StartDragNormalNoteCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldHeadCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldTailCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldBodyCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideHeadCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideTailCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideBodyCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSelectionCommand {
    DragStartInfo common_info;
    std::unordered_set<entt::entity> selection;
};

// 当拖拽结束时的命令
struct EndDragCommand {
    // 可以包含最终鼠标位置等信息
    glm::vec2 final_mouse_pos;
};

// 标记删除指令
struct MarkDeleteCommand {
    const MeshPartInfo hit_info;
};

// 标记删除指令
struct ConfirmDeleteCommand {
    bool confirm{false};
};

// 未来可以添加点击命令等
// struct ClickCommand {
//     const MeshPartInfo* hit_info;
// };

// 用 std::variant 将所有命令类型聚合到一个类型中
using ToolCommand =
    std::variant<StartDragNormalNoteCommand, StartDragHoldHeadCommand,
                 StartDragHoldBodyCommand, StartDragHoldTailCommand,
                 StartDragSelectionCommand, EndDragCommand>;

#endif  // MMM_TOOLCOMMAND_HPP
