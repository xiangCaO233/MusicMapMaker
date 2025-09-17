#ifndef MMM_TOOLCOMMAND_HPP
#define MMM_TOOLCOMMAND_HPP

#include <tool/command/HoldCommand.hpp>
#include <tool/command/SlideCommand.hpp>
#include <variant>

struct StartDragNormalNoteCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSelectionCommand {
    DragStartInfo common_info;
    std::unordered_set<entt::entity> selection;
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
                 StartDragHoldNodeCommand, StartDragSlideHeadCommand,
                 StartDragSlideBodyCommand, StartDragSlideTailCommand,
                 StartDragSlideNodeCommand, StartDragSelectionCommand,
                 EndDragCommand>;

#endif  // MMM_TOOLCOMMAND_HPP
