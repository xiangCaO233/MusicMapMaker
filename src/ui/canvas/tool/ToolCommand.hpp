#ifndef MMM_TOOLCOMMAND_HPP
#define MMM_TOOLCOMMAND_HPP

#include <info/NotePart.hpp>
#include <variant>

// 当拖拽开始时的命令
struct StartDragCommand {
    // 包含了实体、部位、句柄等所有上下文信息
    const MeshPartInfo* hit_info;
};

// 当拖拽结束时的命令
struct EndDragCommand {
    // 可以包含最终鼠标位置等信息
    glm::vec2 final_mouse_pos;
};

// 未来可以添加点击命令等
struct ClickCommand {
    const MeshPartInfo* hit_info;
};

// 用 std::variant 将所有命令类型聚合到一个类型中
using ToolCommand =
    std::variant<StartDragCommand, EndDragCommand, ClickCommand>;

#endif  // MMM_TOOLCOMMAND_HPP
