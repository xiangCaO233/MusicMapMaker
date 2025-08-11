#include <qlogging.h>
#include <qobject.h>

#include <QKeyEvent>
#include <canvas/map/MapCanvas.hpp>

void MapCanvas::keyPressEvent(QKeyEvent *e) {
    GLCanvas::keyPressEvent(e);
    qDebug() << "receive key:" << e->key();
}

void MapCanvas::keyReleaseEvent(QKeyEvent *e) {
    GLCanvas::keyReleaseEvent(e);
    qDebug() << "release key:" << e->key();
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e) { GLCanvas::mouseMoveEvent(e); }

void MapCanvas::mousePressEvent(QMouseEvent *e) {
    GLCanvas::mousePressEvent(e);
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *e) {
    GLCanvas::mouseReleaseEvent(e);
}
