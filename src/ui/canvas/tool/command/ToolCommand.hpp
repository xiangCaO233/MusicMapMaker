#ifndef MMM_TOOLCOMMAND_HPP
#define MMM_TOOLCOMMAND_HPP

#include <tool/command/DragCommand.hpp>
#include <variant>

struct StartDragCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSelectionCommand {
    DragStartInfo common_info;
    std::unordered_set<entt::entity> selection;
};

// 清理拖动实体状态
struct ClearDragStateCommand {};

// 创建物件(单键)指令
struct StartCreateNewNormalNoteCommand {};

// 创建物件(复合键)指令
struct StartCreateNewCompositeNoteCommand {};

// 更新创建物件中的节点指令
struct UpdateCreateNodeCommand {};

// 确认创建新物件指令
struct ConfirmCreateNewNoteCommand {};

// 标记删除指令
struct MarkDeleteCommand {
    const MeshPartInfo hit_info;
    std::unordered_set<entt::entity> selection;
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
    std::variant<StartDragCommand, StartDragSelectionCommand, EndDragCommand,
                 ClearDragStateCommand, StartCreateNewNormalNoteCommand,
                 StartCreateNewCompositeNoteCommand, UpdateCreateNodeCommand,
                 ConfirmCreateNewNoteCommand, MarkDeleteCommand,
                 ConfirmDeleteCommand>;

#endif  // MMM_TOOLCOMMAND_HPP
