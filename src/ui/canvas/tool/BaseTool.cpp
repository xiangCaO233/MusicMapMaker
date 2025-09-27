#include <colorful-log.h>

#include <info/MapCanvasInfo.hpp>
#include <map/MapCanvas.hpp>
#include <tool/BaseTool.hpp>

void BaseTool::mousePressEvent(QMouseEvent* e) {
    auto button = e->button();
    auto buttons = e->buttons();

    glm::vec2 mousePos = {e->pos().x(), e->pos().y()};

    toolInteractionState->updateMousePress(mousePos, button, buttons);

    auto mouseState = tool_interaction_state()->getMouseState();

    // 记录鼠标按下时的所处区域
    pressArea = mouseState.area;
    XINFO("鼠标按下,记录当前区域:[" + to_string(pressArea) + "]")

    if (pressArea == MouseArea::PREVIEW) {
        // 左键拖动移动预览区内的主轨道位置
        // 右键拖动移动预览区内的主轨道位置
        tool_command_queue()->push(StartDragPreviewCommand{
            glm::vec2{e->pos().x(), e->pos().y()}, e->modifiers(), button});
    }
}

void BaseTool::mouseMoveEvent(QMouseEvent* e) {
    glm::vec2 mousePos = {e->pos().x(), e->pos().y()};
    auto mouseState = tool_interaction_state()->getMouseState();
    toolInteractionState->updateMouseMove(mousePos, e->buttons());
    if (pressArea == MouseArea::PREVIEW) {
        XINFO("鼠标在预览区拖动");
        tool_command_queue()->push(
            DragPreviewUpdateCommand{mouseState.last_pressed_button, mousePos});
    }
}

void BaseTool::mouseReleaseEvent(QMouseEvent* e) {
    toolInteractionState->updateMouseRelease({e->pos().x(), e->pos().y()},
                                             e->button(), e->buttons());
    if (pressArea == MouseArea::PREVIEW) {
        // 触发按下的区域是预览区/触发结束预览区拖拽指令
        tool_command_queue()->push(EndDragPreviewCommand{});
    }

    pressArea = MouseArea::UNKNOWN;
}

void BaseTool::keyPressEvent(QKeyEvent* e) {}

void BaseTool::keyReleaseEvent(QKeyEvent* e) {}

void BaseTool::wheelEvent(QWheelEvent* e) {}
