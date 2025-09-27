#ifndef MMM_TOOLCOMMAND_HPP
#define MMM_TOOLCOMMAND_HPP

#include <tool/command/DragCommand.hpp>
#include <variant>

// 开始拖拽预览区指令
struct StartDragPreviewCommand {
    DragStartInfo common_info;
    Qt::MouseButton start_button;
};

// 拖拽预览区位置更新指令
struct DragPreviewUpdateCommand {
    Qt::MouseButton start_button;
    glm::vec2 dragging_pos;
};

// 结束拖拽预览区指令
struct EndDragPreviewCommand {
    Qt::MouseButton trigger_button;
};

// 开始拖拽指令
struct StartDragCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
    bool move_only{false};
};

// 开始选择指令
struct StartSelectCommand {
    DragStartInfo common_info;
    Qt::MouseButton trigger_button;
    // 追加模式(不清空已有选择项)
    bool append{false};
};

// 更新选择区域指令
struct UpdateSelectAreaCommand {
    QFlags<Qt::MouseButton> current_buttons;
};

// 结束选择指令
struct EndSelectCommand {
    Qt::MouseButton end_button;
};

// 开始拖拽选择区域指令
struct StartDragSelectionCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
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

// 复制指令
struct CopyCommand {
    std::unordered_set<entt::entity> entities;
};

// 剪切指令
struct CutCommand {
    std::unordered_set<entt::entity> entities;
};

// 粘贴指令
struct PasteCommand {};

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
    std::variant<StartDragPreviewCommand, DragPreviewUpdateCommand,
                 EndDragPreviewCommand, StartDragCommand, StartSelectCommand,
                 UpdateSelectAreaCommand, EndSelectCommand,
                 StartDragSelectionCommand, EndDragCommand,
                 ClearDragStateCommand, StartCreateNewNormalNoteCommand,
                 StartCreateNewCompositeNoteCommand, UpdateCreateNodeCommand,
                 ConfirmCreateNewNoteCommand, CopyCommand, CutCommand,
                 PasteCommand, MarkDeleteCommand, ConfirmDeleteCommand>;

#endif  // MMM_TOOLCOMMAND_HPP
