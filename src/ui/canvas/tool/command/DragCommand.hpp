#ifndef MMM_DRAGCOMMAND_HPP
#define MMM_DRAGCOMMAND_HPP

#include <qnamespace.h>

#include <info/NotePart.hpp>

struct DragStartInfo {
    glm::vec2 start_mouse_pos;
    QFlags<Qt::KeyboardModifier> modifiers;
};

// 当拖拽结束时的命令
struct EndDragCommand {
    // 可以包含最终鼠标位置等信息
    glm::vec2 final_mouse_pos;
};

#endif  // MMM_DRAGCOMMAND_HPP
