#ifndef MMM_SLIDECOMMAND_HPP
#define MMM_SLIDECOMMAND_HPP

#include <tool/command/DragCommand.hpp>

struct StartDragSlideHeadCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideBodyCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideTailCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

struct StartDragSlideNodeCommand {
    DragStartInfo common_info;
    const MeshPartInfo hit_info;
};

#endif  // MMM_SLIDECOMMAND_HPP
