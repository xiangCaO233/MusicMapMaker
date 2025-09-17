#ifndef MMM_HOLDCOMMAND_HPP
#define MMM_HOLDCOMMAND_HPP

#include <tool/command/DragCommand.hpp>

struct StartDragHoldHeadCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldBodyCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldTailCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragHoldNodeCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

#endif  // MMM_HOLDCOMMAND_HPP
