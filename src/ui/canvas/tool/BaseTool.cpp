#include <tool/BaseTool.hpp>

void BaseTool::mousePressEvent(QMouseEvent* e) {
    toolInteractionState->updateMousePress({e->pos().x(), e->pos().y()},
                                           e->button(), e->buttons());
}

void BaseTool::mouseMoveEvent(QMouseEvent* e) {
    toolInteractionState->updateMouseMove({e->pos().x(), e->pos().y()},
                                          e->buttons());
}

void BaseTool::mouseReleaseEvent(QMouseEvent* e) {
    toolInteractionState->updateMouseRelease({e->pos().x(), e->pos().y()},
                                             e->button(), e->buttons());
}

void BaseTool::keyPressEvent(QKeyEvent* e) {}

void BaseTool::keyReleaseEvent(QKeyEvent* e) {}

void BaseTool::wheelEvent(QWheelEvent* e) {}
