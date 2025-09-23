#include <tool/BaseTool.hpp>

void BaseTool::mouseMoveEvent(QMouseEvent* e) {
    toolInteractionState->updateMouse({e->pos().x(), e->pos().y()},
                                      e->buttons());
}

void BaseTool::keyPressEvent(QKeyEvent* e) {}

void BaseTool::keyReleaseEvent(QKeyEvent* e) {}

void BaseTool::wheelEvent(QWheelEvent* e) {}
