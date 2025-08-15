#include <QKeyEvent>
#include <QObject>
#include <canvas/map/MapCanvas.hpp>

void MapCanvas::keyPressEvent(QKeyEvent *e) {
    GLCanvas::keyPressEvent(e);
    current_tool->keyPressEvent(e);
}

void MapCanvas::keyReleaseEvent(QKeyEvent *e) {
    GLCanvas::keyReleaseEvent(e);
    current_tool->keyReleaseEvent(e);
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e) {
    GLCanvas::mouseMoveEvent(e);
    current_tool->mouseMoveEvent(e);
}

void MapCanvas::mousePressEvent(QMouseEvent *e) {
    GLCanvas::mousePressEvent(e);
    current_tool->mousePressEvent(e);
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *e) {
    GLCanvas::mouseReleaseEvent(e);
    current_tool->mouseReleaseEvent(e);
}
