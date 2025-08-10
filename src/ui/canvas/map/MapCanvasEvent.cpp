#include <qlogging.h>
#include <qobject.h>

#include <QKeyEvent>
#include <canvas/map/MapCanvas.hpp>

void MapCanvas::keyPressEvent(QKeyEvent *e) {
    QSignalBlocker blocker(this);
    qDebug() << "receive key:" << e->key();
}

void MapCanvas::keyReleaseEvent(QKeyEvent *e) {
    QSignalBlocker blocker(this);
    qDebug() << "release key:" << e->key();
}
