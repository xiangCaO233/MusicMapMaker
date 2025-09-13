#include <tool/BaseTool.hpp>

// 从Canvas转发过来的事件
void BaseTool::mousePressEvent(QMouseEvent* e) {
    // toolInteractionState->updateMouse({e->pos().x(), e->pos().y()},
    //                                   e->buttons());
}

void BaseTool::mouseMoveEvent(QMouseEvent* e) {
    toolInteractionState->updateMouse({e->pos().x(), e->pos().y()},
                                      e->buttons());
}

void BaseTool::mouseReleaseEvent(QMouseEvent* event) {}

void BaseTool::keyPressEvent(QKeyEvent* e) {}

void BaseTool::keyReleaseEvent(QKeyEvent* e) {}
